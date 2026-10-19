/**
 * Smoke-test the log callback in the libghostty-vt Wasm static archive.
 *
 *   zig build test-lib-vt-wasm-log-build -Demit-lib-vt \
 *     -Dtarget=wasm32-freestanding -Doptimize=Debug
 *   node test/wasm-log.mjs zig-out/bin/wasm-log.wasm
 */

import assert from "node:assert/strict";
import fs from "node:fs/promises";

const path = process.argv[2];
if (path === undefined) {
  console.error("usage: node test/wasm-log.mjs <wasm-log.wasm>");
  process.exit(2);
}

const module = await WebAssembly.compile(await fs.readFile(path));
// Only the fixture's callback bridge may be imported, not the standalone
// module's env.log: archive consumers must control logging through the C API.
assert.deepEqual(WebAssembly.Module.imports(module), [
  { module: "test", name: "log", kind: "function" },
]);
const messages = [];
const { exports } = await WebAssembly.instantiate(module, {
  test: {
    log(userdata, level, scope, scopeLen, message, messageLen) {
      const bytes = new Uint8Array(exports.memory.buffer);
      const decoder = new TextDecoder();
      messages.push({
        userdata,
        level,
        scope: decoder.decode(bytes.subarray(scope, scope + scopeLen)),
        message: decoder.decode(bytes.subarray(message, message + messageLen)),
      });
    },
  },
});

const slot = exports.ghostty_wasm_alloc_opaque();
assert.notEqual(slot, 0);
assert.equal(exports.ghostty_terminal_new(0, slot, 83, 27), 0);
const terminal = exports.ghostty_wasm_take_opaque(slot);
assert.notEqual(terminal, 0);
const input = new TextEncoder().encode("\x1b[99K");
const ptr = exports.ghostty_wasm_alloc(input.length);
assert.notEqual(ptr, 0);
new Uint8Array(exports.memory.buffer, ptr, input.length).set(input);

// Invalid erase-line mode emits a warning in both Debug and release builds.
exports.ghostty_terminal_vt_write(terminal, ptr, input.length);
assert.deepEqual(messages, []);
assert.equal(exports.ghostty_sys_set(0, ptr), 0); // USERDATA
for (const enabled of [true, false, true]) {
  assert.equal(
    exports.ghostty_sys_set(2, enabled ? exports.callback_pointer() : 0), // LOG
    0,
  );
  messages.length = 0;
  exports.ghostty_terminal_vt_write(terminal, ptr, input.length);
  assert.equal(messages.length, enabled ? 1 : 0);
  if (enabled) {
    assert.equal(messages[0].userdata, ptr);
    assert.equal(messages[0].level, 1); // WARNING
    assert.equal(messages[0].scope, "stream");
    assert.match(messages[0].message, /^invalid erase line command:/);
  }
}

assert.equal(exports.ghostty_sys_set(2, 0), 0);
assert.equal(exports.ghostty_sys_set(0, 0), 0);
exports.ghostty_terminal_free(terminal);
exports.ghostty_wasm_free(ptr, input.length);
exports.ghostty_wasm_free_opaque(slot);
console.log(`Wasm log callback smoke test passed: ${path}`);
