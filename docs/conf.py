import sys
import os
import shlex
import subprocess
import sphinx_rtd_theme

read_the_docs_build = os.environ.get('READTHEDOCS', None) == 'True'

if read_the_docs_build:
    subprocess.call('doxygen', shell=True)

extensions = ['breathe', 'guzzle_sphinx_theme']
breathe_projects = { 'Move38-Arduino-Platform': 'xml' }
breathe_default_project = "Move38-Arduino-Platform"
source_suffix = '.rst'
master_doc = 'index'
project = u'Move38-Arduino-Platform'
copyright = u'2017, Move38'
author = u'Move38'
version = '1.0'
release = '1.0'
language = None
exclude_patterns = ['_build']
pygments_style = 'sphinx'
todo_include_todos = False
htmlhelp_basename = 'Move38-Arduino-Platformdoc'
latex_elements = {
}
latex_documents = [
  (master_doc, 'Move38-Arduino-Platform.tex', u'Move38-Arduino-Platform Documentation',
   u'Move38-Arduino-Platform', 'manual'),
]

html_theme = "sphinx_rtd_theme"
templates_path = ['templates']
html_static_path = ['static']
html_show_sphinx = False
html_show_sourcelink = False
