#!/usr/bin/env node

const { execSync } = require('child_process');
const fs = require('fs');
const path = require('path');
const os = require('os');

console.log('=== Installing DragonFlyDB Client ===\n');

const platform = os.platform();
const binDir = path.join(__dirname, 'bin');
const binaryName = platform === 'win32' ? 'dragonfly-cli.exe' : 'dragonfly-cli';
const binaryPath = path.join(binDir, binaryName);

// Create bin directory if it doesn't exist
if (!fs.existsSync(binDir)) {
    fs.mkdirSync(binDir, { recursive: true });
}

// Check if gcc is available
try {
    execSync('gcc --version', { stdio: 'ignore' });
} catch (error) {
    console.error('Error: gcc not found. Please install gcc first.');
    console.error('');
    if (platform === 'win32') {
        console.error('Windows: Install MinGW from https://www.mingw-w64.org/');
    } else if (platform === 'darwin') {
        console.error('macOS: Install Xcode Command Line Tools with: xcode-select --install');
    } else {
        console.error('Linux: Install gcc with: sudo apt install gcc (Ubuntu/Debian) or sudo yum install gcc (RedHat/CentOS)');
    }
    process.exit(1);
}

// Compile the client
console.log('Compiling DragonFlyDB client...');

const sourceFiles = ['client.c', 'network.c', 'protocol.c'];
const compileFlags = platform === 'win32' ? '-lws2_32 -liphlpapi' : '-pthread';

try {
    const compileCommand = `gcc -Wall -O2 -o "${binaryPath}" ${sourceFiles.join(' ')} ${compileFlags}`;
    execSync(compileCommand, { stdio: 'inherit', cwd: __dirname });
    
    // Make executable on Unix-like systems
    if (platform !== 'win32') {
        fs.chmodSync(binaryPath, 0o755);
    }
    
    console.log('\n✓ Installation successful!\n');
    console.log('Usage:');
    console.log('  dragonfly-cli                    # Connect to localhost:6379');
    console.log('  dragonfly-cli <host> <port>      # Connect to remote server');
    console.log('');
    console.log('Example:');
    console.log('  dragonfly-cli 51.21.226.149 6379');
    console.log('');
    
} catch (error) {
    console.error('\nError: Compilation failed');
    console.error(error.message);
    process.exit(1);
}
