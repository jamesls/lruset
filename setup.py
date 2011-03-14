from distutils.core import setup, Extension

setup(
    name='lruset',
    version='0.1',
    description="A fixed size set with LRU eviction.",
    license='BSD',
    author='James Saryerwinnie',
    author_email='jlsnpi@gmail.com',
    py_modules=['lruset'],
    ext_modules=[Extension('clruset', sources=['clruset.c'])],
    classifiers=[
        'Development Status :: 3 - Alpha'
        'Intended Audience :: Developers',
        'License :: OSI Approved :: BSD License',
    ]
)
