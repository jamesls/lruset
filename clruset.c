#include <Python.h>
#include <structmember.h>


typedef struct node {
    PyObject *data;
    long index;
    struct node *next;
    struct node *previous;
} lrusetnode;

typedef struct lrusetobject {
    PyObject_HEAD
    PyObject *lookup;
    long max_size;
    long current_size;
    long *free_nodes;
    long free_index;
    lrusetnode *head;
    lrusetnode *tail;
    lrusetnode *nodes;
} lrusetobject;

static int lruset_seq_contains(PyObject *op, PyObject *key);


/* Set a key error with the specified argument, wrapping it in a
 * tuple automatically so that tuple keys are not unpacked as the
 * exception arguments. */
static void
set_key_error(PyObject *arg)
{
    PyObject *tup;
    tup = PyTuple_Pack(1, arg);
    if (!tup)
        return; /* caller will expect error to be set anyway */
    PyErr_SetObject(PyExc_KeyError, tup);
    Py_DECREF(tup);
}


/* Iterator object for LRUSet */
typedef struct {
	PyObject_HEAD
	lrusetobject *lruset_ob;
	lrusetnode *li_current;
} lrusetiterobject;


static void
lrusetiter_dealloc(lrusetiterobject *li)
{
	Py_DECREF(li->lruset_ob);
	PyObject_Del(li);
}

static PyObject *
lrusetiter_iternext(lrusetiterobject *li)
{
	if (li->li_current == NULL) {
		/* Iteration is finished */
		return NULL;
	}
	lrusetnode *current = li->li_current;
	li->li_current = current->next;
	Py_INCREF(current->data);
	return current->data;
}

static PyTypeObject lrusetiter_type = {
        PyObject_HEAD_INIT(NULL)
        0,
	"lrusetiterator",			/* tp_name */
	sizeof(lrusetiterobject),		/* tp_basicsize */
	0,					/* tp_itemsize */
	/* methods */
	(destructor)lrusetiter_dealloc, 		/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	0,					/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,			/* tp_flags */
 	0,					/* tp_doc */
 	0,					/* tp_traverse */
 	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	PyObject_SelfIter,			/* tp_iter */
	(iternextfunc)lrusetiter_iternext,	/* tp_iternext */
	0,					/* tp_methods */
	0,
};

static PyObject *
lruset_iter(lrusetobject *l)
{
	lrusetiterobject *li = PyObject_New(lrusetiterobject, &lrusetiter_type);
	if (li == NULL)
		return NULL;
	li->li_current = NULL;
	Py_INCREF(l);
	li->lruset_ob = l;
	if (l->head != NULL) {
		li->li_current = l->head;
	}
	return (PyObject *)li;
}


static int
lruset_init(lrusetobject *self, PyObject *args, PyObject *kwds)
{
    static char *argnames[] = {"max_size", NULL};
    long max_size;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "l", argnames, &max_size)) {
        return -1;
    }
    self->lookup = PyDict_New();
    if (self->lookup == NULL) {
        Py_DECREF(self);
        return -1;
    }
    self->max_size = max_size;
    self->current_size = 0;
    self->free_index = max_size - 1;
    self->head = NULL;
    self->tail = NULL;
    /* Preallocate the node objects into an array */
    int i;
    lrusetnode *temp = (lrusetnode *)PyMem_New(lrusetnode, max_size);
    if (temp == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    self->free_nodes = (long *)PyMem_New(long, max_size);
    if (self->free_nodes == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    for (i = 0; i < max_size; i++) {
        temp[i].data = NULL;
        temp[i].next = NULL;
        temp[i].previous = NULL;
        temp[i].index = -1;
    }
    for (i = 0; i < max_size; i++) {
        self->free_nodes[i] = max_size - (i + 1);
    }
    self->nodes = temp;
    return 0;
}

static void
lruset_dealloc(lrusetobject *self)
{
    int i;
    PyObject *tmp;
    for (i = 0; i < self->max_size; i++) {
        if (self->nodes[i].data != NULL) {
            tmp = self->nodes[i].data;
            self->nodes[i].data = NULL;
            Py_DECREF(tmp);
        }
    }
    Py_DECREF(self->lookup);
    PyMem_Free(self->free_nodes);
    PyMem_Free(self->nodes);
    self->ob_type->tp_free((PyObject *)self);
}

static PyObject *
lruset_add(lrusetobject *self, PyObject *item)
{
    if (lruset_seq_contains((PyObject *)self, item)) {
        Py_RETURN_NONE;
    }
    lrusetnode *node;
    if (self->current_size >= self->max_size) {
        /* We need to remove the LRU element and reuse
	 * that node for the new item we'd like to insert,
	 * however if the maximum size is only 1, we just
	 * reuse the head node.
	 **/
        node = self->head;
	if (self->max_size > 1) {
            self->head = node->next;
	    self->head->previous = NULL;
            node->next = NULL;
	} else {
            self->head = NULL;
	}
        if (PyDict_DelItem(self->lookup, node->data) < 0) {
            return NULL;
        }
        Py_DECREF(node->data);
        node->data = NULL;
        assert(node->index != -1);
        self->current_size--;
    } else {
        assert(self->free_index >= 0);
        node = &self->nodes[self->free_index];
	/* Verify the node is properly "uninitialized" */
	assert(node->data == NULL);
	assert(node->index == -1);
	assert(node->next == NULL);
	assert(node->previous == NULL);
        node->index = self->free_index--;
    }
    node->data = item;
    Py_INCREF(node->data);
    PyObject *py_index = PyInt_FromLong(node->index);
    if (PyDict_SetItem(self->lookup, item, py_index) < 0) {
        Py_DECREF(py_index);
        return NULL;
    }
    /* Transfer ownership to the lookup dict. */
    Py_DECREF(py_index);
    /* Add the node to the end of the list. */
    if (self->head != NULL && self->tail != NULL) {
        node->previous = self->tail;
        self->tail->next = node;
        self->tail = node;
    } else if (self->tail == NULL && self->head != NULL) {
        assert(self->current_size == 1);
        self->head->next = node;
        node->previous = self->head;
        self->tail = self->head;
        self->tail = node;
    } else if (self->head == NULL) {
        assert(self->current_size == 0);
        self->head = node;
    }
    self->current_size++;
    Py_RETURN_NONE;
}


static PyObject *
lruset_remove(lrusetobject *self, PyObject *item)
{
    PyObject *py_index = PyDict_GetItem(self->lookup, item);
    if (py_index == NULL) {
        set_key_error(item);
        return NULL;
    }
    long index;
    if ((index = PyInt_AsLong(py_index)) == -1) {
        if (PyErr_Occurred()) {
            return NULL;
        }
    }
    lrusetnode *node = &self->nodes[index];
    if (self->head == node && self->head->next == NULL) {
        self->head = NULL;
    }
    else if (self->head == node && self->head->next != NULL) {
        self->head = node->next;
        node->next->previous = NULL;
        node->next = NULL;
	if (self->head == self->tail) {
            self->tail = NULL;
	}
    } else if (self->tail == node) {
        if (node->previous != self->head) {
            self->tail = node->previous;
        } else {
            self->tail = NULL;
        }
        node->previous->next = NULL;
        node->previous = NULL;
    } else {
        node->previous->next = node->next;
        node->next->previous = node->previous;
    }
    /* Now clean up the actual node itself */
    if (PyDict_DelItem(self->lookup, item) < 0) {
        return NULL;
    }
    self->current_size -= 1;
    self->free_nodes[++self->free_index] = node->index;
    node->index = -1;
    Py_DECREF(node->data);
    node->data = NULL;
    assert(node->data == NULL);
    assert(node->previous == NULL);
    assert(node->next == NULL);
    assert(node->index == -1);
    Py_RETURN_NONE;
}

/* support the "mykey in lruset" syntax */
static int
lruset_seq_contains(PyObject *op, PyObject *key)
{
    lrusetobject *self = (lrusetobject *)op;
    PyObject *py_index = PyDict_GetItem(self->lookup, key);
    if (py_index == NULL) {
        return 0;
    }
    long index;
    if ((index = PyInt_AsLong(py_index)) == -1) {
        if (PyErr_Occurred()) {
            return -1;
        }
    }
    lrusetnode *node = &self->nodes[index];
    /* Move node to the end of the list */
    if ((self->head == node && self->tail == NULL) ||
        self->tail == node) {
        /* Do nothing */
    } else if (self->head == node) {
        lrusetnode *new_head = self->head->next;
	assert(new_head != NULL);
	node->next = NULL;
	new_head->previous = NULL;
	self->head = new_head;
	self->tail->next = node;
	node->previous = self->tail;
	self->tail = node;
    } else {
        node->previous->next = node->next;
	node->next->previous = node->previous;
	node->previous = self->tail;
	node->next = NULL;
	self->tail->next = node;
	self->tail = node;
    }
    return 1;
}

static Py_ssize_t
lruset_seq_length(PyObject *op)
{
    lrusetobject *mp = (lrusetobject *)op;
    return PyDict_Size(mp->lookup);
}

static PySequenceMethods lruset_as_sequence = {
    lruset_seq_length,  /* sq_length */
    0,          /* sq_concat */
    0,          /* sq_repeat */
    0,          /* sq_item */
    0,          /* sq_slice */
    0,          /* sq_ass_item */
    0,          /* sq_ass_slice */
    lruset_seq_contains,    /* sq_contains */
    0,          /* sq_inplace_concat */
    0,          /* sq_inplace_repeat */
};

static PyMemberDef lruset_members[] = {
    {"max_size", T_LONG, offsetof(lrusetobject, max_size), 0, "Maximum size"},
    {"current_size", T_LONG, offsetof(lrusetobject, current_size), 0, "Current size"},
    {NULL}
};

static PyMethodDef lruset_methods[] = {
    {"add", (PyCFunction)lruset_add, METH_O,
        PyDoc_STR("adds an element to the set")},
    {"remove", (PyCFunction)lruset_remove, METH_O,
        PyDoc_STR("removes an element to the set")},
    {NULL, NULL},
};

static PyTypeObject lruset_type = {
    PyObject_HEAD_INIT(NULL)
    0,
    "clruset.LRUSet",       /* tp_name */
    sizeof(lrusetobject),       /* tp_basicsize */
    0,              /* tp_itemsize */
    /* methods */
    (destructor)lruset_dealloc, /* tp_dealloc */
    0,              /* tp_print */
    0,              /* tp_getattr */
    0,              /* tp_setattr */
    0,              /* tp_compare */
    0,              /* tp_repr */
    0,              /* tp_as_number */
    &lruset_as_sequence,        /* tp_as_sequence */
    0,              /* tp_as_mapping */
    0,              /* tp_hash */
    0,              /* tp_call */
    0,              /* tp_str */
    PyObject_GenericGetAttr,    /* tp_getattro */
    0,              /* tp_setattro */
    0,              /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,     /* tp_flags */
    0,              /* tp_doc */
    0,              /* tp_traverse */
    0,              /* tp_clear */
    0,              /* tp_richcompare */
    0,              /* tp_weaklistoffset*/
    (getiterfunc)lruset_iter,             /* tp_iter */
    0,              /* tp_iternext */
    lruset_methods,         /* tp_methods */
    lruset_members,         /* tp_members */
    0,              /* tp_getset */
    0,              /* tp_base */
    0,              /* tp_dict */
    0,              /* tp_descr_get */
    0,              /* tp_descr_set */
    0,              /* tp_dictoffset */
    (initproc)lruset_init,      /* tp_init */
    0,              /* tp_alloc */
    PyType_GenericNew,          /* tp_new */
    0,              /* tp_free */
};



PyMODINIT_FUNC
initclruset(void)
{
    if (PyType_Ready(&lruset_type) < 0)
        return;
    PyObject *module = Py_InitModule3("clruset", NULL,
                "C Extension for Least Recently Used Set.");
    Py_INCREF(&lruset_type);
    if (PyModule_AddObject(module, "LRUSet",
                (PyObject *) &lruset_type) < 0)
        return;
}
