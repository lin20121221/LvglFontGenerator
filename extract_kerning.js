#!/usr/bin/env node
/**
 * Extract kerning data from a font file using opentype.js
 * Usage: node extract_kerning.js <font_path> <size> <characters>
 */

const fs = require('fs');
const path = require('path');

// Try to load opentype.js from lv_font_conv's node_modules
let opentype;
const possiblePaths = [
    path.join(__dirname, '../lv_font_conv/node_modules/opentype.js'),
    path.join(__dirname, '../../lv_font_conv/node_modules/opentype.js'),
    'Z:/LvglFontUtility/lv_font_conv/node_modules/opentype.js',
    'opentype.js'
];

let loaded = false;
for (const modulePath of possiblePaths) {
    try {
        opentype = require(modulePath);
        loaded = true;
        break;
    } catch (e) {
        // Try next path
    }
}

if (!loaded) {
    console.error('Error: opentype.js not found.');
    console.error('Tried paths:', possiblePaths);
    console.error('Please run: cd Z:/LvglFontUtility/lv_font_conv && npm install');
    process.exit(1);
}

if (process.argv.length < 5) {
    console.error('Usage: node extract_kerning.js <font_path> <size> <characters>');
    console.error('Example: node extract_kerning.js font.ttf 48 "ABC"');
    process.exit(1);
}

const fontPath = process.argv[2];
const fontSize = parseInt(process.argv[3]);
const characters = process.argv[4];

// Load font
const font = opentype.loadSync(fontPath);

console.log(`Font: ${font.names.fullName.en}`);
console.log(`Units per EM: ${font.unitsPerEm}`);
console.log(`Font size: ${fontSize}`);
console.log(`Characters: ${characters}`);
console.log();

// Get unique characters and sort by Unicode code point (same as C++)
const chars = Array.from(new Set(characters)).sort((a, b) => {
    return a.charCodeAt(0) - b.charCodeAt(0);
});
console.log(`Total unique characters: ${chars.length}`);
console.log();

// Extract kerning for all pairs
const kerningPairs = [];
let nonZeroCount = 0;

for (let i = 0; i < chars.length; i++) {
    for (let j = 0; j < chars.length; j++) {
        const char1 = chars[i];
        const char2 = chars[j];

        const glyph1 = font.charToGlyph(char1);
        const glyph2 = font.charToGlyph(char2);

        // Get kerning value in font units
        const kernValue = font.getKerningValue(glyph1, glyph2);

        if (kernValue !== 0) {
            // Convert to pixels at the given font size
            const kernPixels = kernValue * fontSize / font.unitsPerEm;

            // Convert to LVGL format (1/16 pixels, kern_scale=16)
            const kernLVGL = Math.round(kernPixels * 16);

            kerningPairs.push({
                left: char1,
                right: char2,
                leftIndex: i,
                rightIndex: j,
                valueUnits: kernValue,
                valuePixels: kernPixels,
                valueLVGL: kernLVGL
            });

            nonZeroCount++;

            // Print first 10 pairs
            if (nonZeroCount <= 10) {
                console.log(`  ${char1}-${char2}: class(${i+1},${j+1}) units=${kernValue} px=${kernPixels.toFixed(2)} lvgl=${kernLVGL}`);
            }
        }
    }
}

console.log();
console.log(`Total non-zero kerning pairs: ${nonZeroCount}`);

// Output as JSON for C++ to parse
const output = {
    fontName: font.names.fullName.en,
    unitsPerEm: font.unitsPerEm,
    fontSize: fontSize,
    characters: chars,
    kerningPairs: kerningPairs
};

// Write to file
const outputPath = fontPath.replace(/\.[^.]+$/, '_kerning.json');
fs.writeFileSync(outputPath, JSON.stringify(output, null, 2));
console.log();
console.log(`Kerning data written to: ${outputPath}`);
