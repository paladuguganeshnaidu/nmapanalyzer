# nmap_scanner

Minimal C project skeleton for `nmap_scanner`.

Structure:
- include/: public headers
- src/: implementation
- www/: web UI static files
- data/: runtime data (contains an empty `scan_results.db` placeholder)
- scripts/: helper SQL scripts

Build:

On a system with `gcc` installed, run:

```powershell
cd "c:/Users/ganes/OneDrive/Desktop/Nmap analysis/nmap_scanner"
make
```

This produces a `nmap_scanner` binary.

Notes:
- All C files are stubs/placeholders and meant to be expanded.
- `scripts/init_db.sql` contains a simple table definition for storing scan results.
