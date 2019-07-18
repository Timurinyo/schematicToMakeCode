#pragma once

#include <string>

std::string tsCode =
R"s(const bufLen : number = buf.length / 2;
let cursor : number = 0;

let worldX : number = 0;
let worldY : number = 0;
let worldZ : number = 0;
const skipAir: boolean = true;

function codeToNum(charCode : number) : number {
	if (charCode >= 48 && charCode <= 57) return charCode - 48;
	if (charCode >= 65 && charCode <= 70) return charCode - 65 + 10;
	if (charCode >= 97 && charCode <= 102) return charCode - 97 + 10;
	return 0;
}
function nextByte() : number {
	const result : number = (codeToNum(buf.charCodeAt(cursor*2)) << 4) | codeToNum(buf.charCodeAt(cursor*2+1));
	++cursor;
	return result;
}

function nextVarNum() : number {
	let position : number = 0;
	let value : number = 0;
	while(true) {
		const byte : number = nextByte();
		value |= (byte & 0b1111111) << position;
		position += 7;
		if (byte & 0b10000000) return value;
	}
}

function runNextCommand() : void {
	const blockId   : number = nextByte();
	const blockData : number = nextByte();
	const runStartX : number = worldX + nextVarNum();
	const runStartY : number = worldY + nextVarNum();
	const runStartZ : number = worldZ + nextVarNum();
	const runLength : number = nextVarNum();

	const fullBlockId : number = (blockData << 16) | blockId;

	if (blockId == 0 && skipAir) return;

	if (runLength)
		shapes.line(fullBlockId,
			positions.createWorld(runStartX, runStartY, runStartZ),
			positions.createWorld(runStartX+runLength-1, runStartY, runStartZ));
	else
		blocks.place(fullBlockId,
			positions.createWorld(runStartX, runStartY, runStartZ));
}

player.onChat("jump", function (x : number, y:number, z:number) {
	worldX = x;
	worldY = y;
	worldZ = z;
	while(cursor < bufLen) runNextCommand();
	player.say("Import done )s";

std::string tsCode2 = R"s(");
});
)s";

