import shutil, os, sys, site as _site

# Auto-detect site-packages path
site = os.path.join(_site.getsitepackages()[0], 'code_review_graph')
# Auto-detect source path (relative to this script)
src = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'code-review-graph-main', 'code_review_graph')

shutil.rmtree(site, ignore_errors=True)
shutil.copytree(src, site, ignore=shutil.ignore_patterns('__pycache__', '*.pyc', '.beads', '.github', '.serena', 'eval'))
print('Synced')
print(f'  Source: {src}')
print(f'  Target: {site}')
