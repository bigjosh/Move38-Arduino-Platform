import sys
import os
import shlex
import subprocess
import guzzle_sphinx_theme

read_the_docs_build = os.environ.get('READTHEDOCS', None) == 'True'

if read_the_docs_build:
    subprocess.call('doxygen', shell=True)

extensions = ['breathe']
breathe_projects = { 'Move38-Arduino-Platform': 'xml' }
breathe_default_project = "Move38-Arduino-Platform"
templates_path = ['_templates']
source_suffix = '.rst'
master_doc = 'index'
project = u'Move38-Arduino-Platform'
copyright = u'2015, Move38-Arduino-Platform'
author = u'Move38-Arduino-Platform'
version = '1.0'
release = '1.0'
language = None
exclude_patterns = ['_build']
pygments_style = 'sphinx'
todo_include_todos = False
html_static_path = ['_static']
htmlhelp_basename = 'Move38-Arduino-Platformdoc'
latex_elements = {
}
latex_documents = [
  (master_doc, 'Move38-Arduino-Platform.tex', u'Move38-Arduino-Platform Documentation',
   u'Move38-Arduino-Platform', 'manual'),
]

html_theme = 'guzzle_sphinx_theme'
html_theme_path = guzzle_sphinx_theme.html_theme_path()
extensions.append("guzzle_sphinx_theme")
