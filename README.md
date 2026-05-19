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
- Automatic fallback page: `./web/index_fallback.html` (served when `index.html` is missing).
- Authentication can be disabled at runtime via `POST /api/settings/write` with
  `{"authRequired": false}` (admin token still required for the change itself).
- Optional auth modes for WebBuilder session experiments can be toggled via
  `POST /api/settings/write`:
  - `digestAuthEnabled: true` -> API accepts `Authorization: Digest ...` and
    maps `username="..."` to demo users.
  - `jwtEnabled: true` -> API accepts demo JWT-like bearer tokens in the form
    `Authorization: Bearer jwt:<username>`.
  - When both are disabled, regular bearer tokens from login are used.
- Demo users:
  - `operator` / `operator123` → `token_operator`
  - `maint` / `maint123` → `token_maintenance`
  - `admin` / `admin123` → `token_admin`
