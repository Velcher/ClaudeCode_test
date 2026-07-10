import shutil, os, sys

site = r'C:\Users\elcher\AppData\Local\Programs\Python\Python311\Lib\site-packages'
src = r'E:\ClaudeCode_test\code-review-graph-main\code-review-graph-main\code_review_graph'

# 1. Remove corrupted directory
target = os.path.join(site, 'code_review_graph')
if os.path.exists(target):
    shutil.rmtree(target)
    print('Removed corrupted:', target)

# 2. Copy fresh source with our fixes
shutil.copytree(src, target, ignore=shutil.ignore_patterns('__pycache__', '*.pyc'))
print('Copied source to:', target)

# 3. Clean corrupted dist-info
for d in os.listdir(site):
    if 'graph' in d and d.startswith('~'):
        full = os.path.join(site, d)
        shutil.rmtree(full)
        print('Removed corrupted dist-info:', full)

# 4. Create entry point script
scripts_dir = os.path.join(os.path.dirname(site), 'Scripts')
os.makedirs(scripts_dir, exist_ok=True)
exe_path = os.path.join(scripts_dir, 'code-review-graph.exe')

# Simple wrapper script
wrapper = os.path.join(scripts_dir, 'code-review-graph-script.py')
with open(wrapper, 'w') as f:
    f.write('''#!python3
from code_review_graph.cli import main
if __name__ == "__main__":
    main()
''')
print('Created wrapper:', wrapper)

print('\nDone! Run: code-review-graph-script.py visualize')
