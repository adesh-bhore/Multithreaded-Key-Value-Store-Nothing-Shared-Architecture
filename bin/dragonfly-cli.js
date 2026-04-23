#!/usr/bin/env node

const { spawn } = require('child_process');
const path = require('path');
const os = require('os');

const platform = os.platform();
const binaryName = platform === 'win32' ? 'dragonfly-cli.exe' : 'dragonfly-cli';
const binaryPath = path.join(__dirname, binaryName);

// Get command-line arguments (skip node and script name)
const args = process.argv.slice(2);

// Spawn the C binary
const child = spawn(binaryPath, args, {
    stdio: 'inherit',
    shell: false
});

child.on('error', (error) => {
    console.error('Error: Failed to start dragonfly-cli');
    console.error(error.message);
    process.exit(1);
});

child.on('exit', (code) => {
    process.exit(code || 0);
});
