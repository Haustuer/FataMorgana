#!/usr/bin/env node

/**
 * Test script for the UDP frame protocol
 *
 * Usage:
 *   node test-protocol.js [pattern] [width] [height] [rgbType]
 *
 * Examples:
 *   node test-protocol.js gradient 16 16 0
 *   node test-protocol.js solid 24 100 0
 *   node test-protocol.js checkerboard 32 32 1
 *   node test-protocol.js rainbow 24 24 1
 */

const http = require('http');

const HOST = process.env.HOST || 'localhost';
const PORT = process.env.HTTP_PORT || 3001;

// Parse command line arguments
const args = process.argv.slice(2);
const pattern = args[0] || 'gradient';
const width = parseInt(args[1]) || 16;
const height = parseInt(args[2]) || 16;
const rgbType = parseInt(args[3]) || 0;

// Pattern-specific options
const patternOptions = {};
if (pattern === 'solid') {
  patternOptions.r = parseInt(args[4]) || 255;
  patternOptions.g = parseInt(args[5]) || 0;
  patternOptions.b = parseInt(args[6]) || 0;
} else if (pattern === 'checkerboard') {
  patternOptions.cellSize = parseInt(args[4]) || 4;
} else if (pattern === 'rainbow') {
  patternOptions.offset = parseInt(args[4]) || 0;
}

const payload = JSON.stringify({
  width,
  height,
  rgbType,
  pattern,
  patternOptions,
});

const options = {
  hostname: HOST,
  port: PORT,
  path: '/api/frame',
  method: 'POST',
  headers: {
    'Content-Type': 'application/json',
    'Content-Length': payload.length,
  },
};

console.log(`\nSending ${pattern} frame to ${HOST}:${PORT}...`);
console.log(`Dimensions: ${width}x${height}`);
console.log(`RGB Type: ${rgbType === 0 ? 'RGB332' : 'RGB565'}`);
if (Object.keys(patternOptions).length > 0) {
  console.log(`Pattern Options:`, patternOptions);
}
console.log('');

const req = http.request(options, (res) => {
  let data = '';

  res.on('data', (chunk) => {
    data += chunk;
  });

  res.on('end', () => {
    try {
      const response = JSON.parse(data);
      if (response.ok) {
        console.log('✓ Frame sent successfully!');
        console.log(`  Frame Counter: ${response.frameCounter}`);
        console.log(`  Chunks: ${response.chunkCount}`);
        console.log(`  Payload: ${response.payloadBytes} bytes`);
        console.log(`  Elapsed: ${response.elapsedMs}ms`);
        console.log('');
      } else {
        console.error('✗ Error:', response.error);
        process.exit(1);
      }
    } catch (e) {
      console.error('✗ Invalid response:', data);
      process.exit(1);
    }
  });
});

req.on('error', (error) => {
  console.error('✗ Request failed:', error.message);
  console.error('\nIs the gradient frame server running?');
  console.error(`Try: node gradientFrameServer.js\n`);
  process.exit(1);
});

req.write(payload);
req.end();
