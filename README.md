# WebBuilder API Demo Server (C + Mongoose)

This is a pure-C demo backend server for testing WebBuilder-generated dashboards.

## Build (Linux)

```bash
make
```

## Build (Windows / MSYS2 UCRT64)

```bash
mingw32-make
```

## Run

```bash
./webbuilder_api_demo
```

Then open:

```text
http://localhost:8000/
```

## Notes

- Static web root: `./web`
- Default entry: `./web/index.html`
- Demo users:
  - `operator` / `operator123` → `token_operator`
  - `maint` / `maint123` → `token_maintenance`
  - `admin` / `admin123` → `token_admin`
