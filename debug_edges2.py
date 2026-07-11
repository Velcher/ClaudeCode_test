import sqlite3
db = sqlite3.connect(r'E:\ClaudeCode_test\verilog-pcie-master\.code-review-graph\graph.db')

nodes = db.execute('SELECT name, qualified_name, kind FROM nodes WHERE kind != "File"').fetchall()

# Check: does target_qualified match node NAME (not qualified_name)?
node_names = set()
name_to_qn = {}
for name, qn, kind in nodes:
    node_names.add(name)
    name_to_qn[name] = qn

print(f'Total non-File nodes: {len(nodes)}')
print()

# Count CALLS edges that resolve by NAME (short) vs by qualified_name (full)
calls = db.execute("SELECT source_qualified, target_qualified, file_path FROM edges WHERE kind='CALLS'").fetchall()
print(f'Total CALLS edges: {len(calls)}')

resolved_by_name = 0
unresolved = []
for src, tgt, fp in calls:
    # Try matching by name (short)
    if tgt in node_names:
        resolved_by_name += 1
    else:
        unresolved.append((src, tgt, fp))

print(f'Resolved by name match: {resolved_by_name}')
print(f'Unresolved even by name: {len(unresolved)}')
if unresolved:
    print()
    print('=== Unresolved targets ===')
    for src, tgt, fp in unresolved[:15]:
        sname = src.split('::')[-1] if '::' in src else src
        print(f'  {sname} -> {tgt}  ({fp.split(chr(92))[-1]})')

# Now: which nodes are TRULY isolated (no edge targets their NAME)?
nodes_with_calls_in = set()
for _, tgt, _ in calls:
    nodes_with_calls_in.add(tgt)

nodes_with_calls_out = set()
for src, _, _ in calls:
    nodes_with_calls_out.add(src)

print()
print('=== TRUE isolated nodes (name not in any CALLS target or source) ===')
count = 0
for name, qn, kind in nodes:
    if name not in nodes_with_calls_in and name not in nodes_with_calls_out:
        fn = qn.split('::')[0].replace('\\','/').split('/')[-1]
        print(f'  {name}  ({fn})')
        count += 1
print(f'\nTrue isolated: {count}')

# Now verify: do example_core_pcie.v's instances show up?
print()
print('=== example_core_pcie outgoing CALLS (from edges table) ===')
for src, tgt, fp in calls:
    if 'example_core_pcie' in src:
        print(f'  -> {tgt}')

db.close()
