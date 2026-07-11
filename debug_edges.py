import sqlite3
db = sqlite3.connect(r'E:\ClaudeCode_test\verilog-pcie-master\.code-review-graph\graph.db')

nodes_qn = set(r[0] for r in db.execute('SELECT qualified_name FROM nodes'))

print('=== Edges with UNRESOLVED targets ===')
count = 0
for row in db.execute("SELECT kind, source_qualified, target_qualified FROM edges WHERE kind='CALLS'"):
    if row[2] not in nodes_qn:
        count += 1
        t = row[2][-50:]
        s = row[1].split('::')[-1] if '::' in row[1] else row[1][-40:]
        if count <= 15:
            print(f'  {s} -> {t}')

print(f'\nTotal unresolved edge targets: {count}')

# Check specific modules
for mod in ['pcie_msix', 'pcie_axil_master', 'pcie_axil_master_minimal', 'pcie_tlp_mux']:
    in_nodes = mod in nodes_qn
    partial = any(mod in q for q in nodes_qn)
    edges_in = db.execute("SELECT COUNT(*) FROM edges WHERE target_qualified=? AND kind='CALLS'", (mod,)).fetchone()[0]
    edges_out = db.execute("SELECT COUNT(*) FROM edges WHERE source_qualified LIKE ? AND kind='CALLS'", (f'%{mod}%',)).fetchone()[0]
    print(f'{mod}: as-is_match={in_nodes}, partial={partial}, edges_in={edges_in}, edges_out={edges_out}')

db.close()
