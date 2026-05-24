## About

Modern cross-platform core for Arma 2 OA game logic and SQL.
For those who need more than the standard: no recompilation, independent component updates, extended analytics and external system integration.

## Screenshot

*Picture*

## Installation

### Server

1. Copy `Osmia/` and `_start_server.cmd` to Arma 2 OA root.
2. Copy `HiveExt.dll` (forwarder) and `Osmia.dll` to your @mod folder.
3. Install `32-bit` ODBC driver (PostgreSQL, MySQL, MariaDB, etc.).
4. Fill in `Osmia/Osmia.ini`. Set `Type` to the driver name from your DB documentation.
5. Run `_start_server.cmd`.

## Configuration

### osmia.ini

```ini
[Database]
Type = PostgreSQL Unicode   ; ODBC driver name
Host = 127.0.0.1            ; database host
Port = 5432                 ; database port
Name = dayz                 ; database name
Username = postgres         ; database user
Password = yourpassword     ; database password

[Logger]
Level = info             ; trace / debug / info / warning / error / fatal
ConsoleLevel = info      ; trace / debug / info / warning / error / fatal, default = Level
Async = false            ; async logging
SeparateConsole = false  ; open separate console window (Windows only)
```

## API

### db.* (osmia.lua)

| Function | Description | Returns |
|---|---|---|
| `db.query(sql)` | Run a SELECT, get all rows at once | `table` of rows, each row is `{field=value, ...}` |
| `db.execute(sql)` | Run INSERT / UPDATE / DELETE | `bool` |
| `db.executeBatch({sql1, sql2, ...})` | Run multiple statements in one transaction | `bool` |
| `db.transaction(fn)` | Call `fn()` inside BEGIN/COMMIT, auto-rollback on error | `bool` |
| `db.queryAsync(sql)` | Start a query on a background thread | `token` (uint32) |
| `db.queryStatus(token)` | Check async query state | `0`=pending, `1`=ready, `-1`=error |
| `db.queryFetch(token)` | Get next row from async result | `table` or `nil` when done |
| `db.queryClose(token)` | Free async result | `bool` |

```lua
-- sync
local rows = db.query("SELECT * FROM Player_DATA WHERE PlayerUID = '123'")
for _, row in ipairs(rows) do
    log.info(row["PlayerName"])
end

-- async
local token = db.queryAsync("SELECT * FROM Object_DATA WHERE Instance = 1")
-- next frame
if db.queryStatus(token) == 1 then
    local row = db.queryFetch(token)
    db.queryClose(token)
end
```

### Handlers

Game logic is defined in `osmia_sql.lua`. Call format: `CHILD:NNN:params`.

### Windows
- Visual Studio 2022 (v143, x86/Win32), C++20
- vcpkg: `lua[cpp]:x86-windows`, `detours:x86-windows-static`
- Open `Source/Osmia.sln`, build `Release|Win32`

### Linux
- GCC 15+ / Clang, CMake 3.20+, C++20
- ODBC: `unixodbc-dev`
- Lua: `liblua5.4-dev` or via vcpkg
- `cmake -B build && cmake --build build`

> Linux build compiles cleanly but has not been tested in production.

## License

MIT

*And... thank you for choosing me 🐝*