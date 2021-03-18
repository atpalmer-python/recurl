from setuptools import setup, Extension


setup(
    name='requests-curl',
    ext_modules=[
        Extension('requests_curl',
            sources=[
                'src/module.c',
            ],
            libraries=['curl'],
        ),
    ],
)

