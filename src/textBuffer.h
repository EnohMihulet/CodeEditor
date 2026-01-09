#pragma once
#include <algorithm>
#include <cassert>

#include "commonTypes.h"

struct LineBuffer {
	u32 size = 0;
	u32 capacity;
	char* text;
	LineBuffer* next = nullptr;
	LineBuffer* prev = nullptr;

	LineBuffer(u32 capacity) {
		this->capacity = capacity;
		text = new char[capacity];
	}

	void append(char c) {
		if (shouldGrowBuffer()) growBuffer();
		if (c == '\t') {
			text[size++] = ' ';
			text[size++] = '>';
			text[size++] = '>';
			text[size++] = ' ';
		}
		else text[size++] = c;
		text[size] = '\0';
	}

	void appendAt(char c, u32 index) {
		if (shouldGrowBuffer()) growBuffer();

		assert(index <= size);
		if (index == size) {
			append(c);
			return;
		}

		for (u32 i = size; i > index; i--) {
			text[i] = text[i-1];
		}
		text[index] = c;
		size++;
	}
	
	void remove() {
		if (size == 0) return;
		text[--size] = 0;
		text[size] = '\0';
	}

	void removeAt(u32 index) {
		if (size == 0) return;

		assert(index < size);
		if (index == size - 1) {
			remove();
			return;
		}
		for (u32 i = index; i < size - 1; i++) {
			text[i] = text[i+1];
		}
		text[--size] = 0;
	}

	void clear() { std::memset(text, 0, size); }

	bool shouldGrowBuffer() {
		return (size + 8 >= capacity);
		
	}

	void growBuffer() {
		char* other = new char[capacity * 2];
		std::copy_n(text, capacity, other);
		delete text;
		text = other;
		capacity = capacity * 2;
	}
};

struct TextBuffer {
	u32 size;
	LineBuffer* front;
	LineBuffer* back;
	const u32 DEFAULT_SIZE = 128;

	TextBuffer(u32 lines) {
		size = lines;
		LineBuffer* curr = new LineBuffer(DEFAULT_SIZE);
		front = curr;
		for (u32 i = 1; i < lines; i++) {
			curr->next = new LineBuffer(DEFAULT_SIZE);
			curr->next->prev = curr;
			curr = curr->next;
		}
		back = curr;
	};

	~TextBuffer() {
		LineBuffer* curr = front;
		while (curr != back) {
			curr = curr->next;
			delete curr->prev;
		}
		delete back;
	}

	LineBuffer* getLineBuffer(u32 index) {
		assert(index < size);
		LineBuffer* curr = front;
		for (u32 i = 0; i < index; i++) {
			curr = curr->next;
		}
		return curr;
	}

	s16 insertAtLine(LineBuffer* nextLine) {
		LineBuffer* newLine = new LineBuffer(DEFAULT_SIZE);

		if (nextLine == nullptr) {
			newLine->prev = back;
			back->next = newLine;
			back = newLine;
			size++;
			return 0;
		}
		else if (nextLine == front) {
			newLine->next = front;
			front->prev = newLine;
			front = newLine;
			size++;
			return 0;
		}

		LineBuffer* prevLine = nextLine->prev;

		newLine->next = nextLine;
		newLine->prev = prevLine;
		prevLine->next = newLine;
		nextLine->prev = newLine;
		size++;
		return 0;
	}

	s16 insertAtIndex(u32 index) {
		assert(index <= size);
		// if (index > size) index = size;

		if (index == size) {
			LineBuffer* newLine = new LineBuffer(DEFAULT_SIZE);
			newLine->prev = back;
			back->next = newLine;
			back = newLine;
			size++;
			return 0;
		}


		LineBuffer* nextLine = front;
		for (u32 i = 0; i < index; i++) nextLine = nextLine->next;

		LineBuffer* newLine = new LineBuffer(DEFAULT_SIZE);

		if (nextLine == front) {
			newLine->next = front;
			front->prev = newLine;
			front = newLine;
			size++;
			return 0;
		}

		LineBuffer* prevLine = nextLine->prev;

		newLine->next = nextLine;
		newLine->prev = prevLine;
		prevLine->next = newLine;
		nextLine->prev = newLine;
		size++;
		return 0;
	}
};
