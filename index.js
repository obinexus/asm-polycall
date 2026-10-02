'use strict';

// Source package entry point: absolute paths for native build tooling. The
// shims themselves are GNU assembly linked against the installed Polycall
// core (>= 1.1.0, binding ABI 1) via pkg-config; this module loads nothing.
const path = require('node:path');

const fromPackageRoot = (...segments) => path.join(__dirname, ...segments);

module.exports = Object.freeze({
  root: __dirname,
  assembly: fromPackageRoot('src', 'asm_polycall.S'),
  publicHeader: fromPackageRoot('include', 'asm_polycall.h'),
  makefile: fromPackageRoot('Makefile'),
  config: fromPackageRoot('asm-polycallrc'),
  manifest: fromPackageRoot('polycall-binding.json'),
  license: fromPackageRoot('LICENSE')
});
