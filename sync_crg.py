import shutil
site = r'C:\Users\elcher\AppData\Local\Programs\Python\Python311\Lib\site-packages\code_review_graph'
src = r'E:\ClaudeCode_test\code-review-graph-main\code-review-graph-main\code_review_graph'
shutil.rmtree(site, ignore_errors=True)
shutil.copytree(src, site, ignore=shutil.ignore_patterns('__pycache__', '*.pyc', '.beads', '.github', '.serena', 'eval'))
print('Synced')
