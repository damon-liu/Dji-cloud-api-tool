# Release Checklist

## Automated Checks

- `pnpm vitest run`
- `pnpm build`
- `cd src-tauri && cargo test`
- `pnpm tauri build`

## Manual Smoke Test

- App starts and shows the main layout.
- Connection dialog opens from the toolbar.
- Add Dock creates a default OSD Topic.
- Add Aircraft under Dock works.
- Topic enable, disable, reorder, add, and delete update the list.
- Raw JSON remains empty before MQTT messages arrive.
- Publish panel rejects invalid JSON.
- Publish panel shows backend publish errors.

## Notes

- Old Qt `config.json` files are not compatible with the new app.
- Rust toolchain is required for `cargo test` and `pnpm tauri build`.
