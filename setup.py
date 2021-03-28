from setuptools import setup, Extension


setup(
    name='requests-curl',
    ext_modules=[
        Extension('requests_curl',
            sources=[
                'src/module.c', 'src/easyadapter.c', 'src/requests.c', 'src/util.c',
                'src/constants.c', 'src/curlwrap.c', 'src/exc.c',
            ],
            libraries=['curl'],
        ),
    ],
)

