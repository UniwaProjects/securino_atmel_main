#include "CharBuffer.h"

//The object is created by specifying the size of the buffer.
//The size cannot be larger than the bufferSize's variable size,
//so if thats the case, the buffer is initialized in the max allowed
//size.
CharBuffer::CharBuffer(uint16_t requested_size)
{
	//Max size is bufferSize's variable's max capacity
	uint16_t max_buffer_size = pow(2, (sizeof(m_buffer_size) * 8));
	//If the size is larger the maximum allowed size
	m_buffer_size = requested_size > max_buffer_size ? max_buffer_size : requested_size;
	//Dynamically allocated the required memory.
	m_buffer = (char *)malloc(m_buffer_size * sizeof(char));
	//Clear fills the buffer with zeroes, which is good for initialization
	clear();
}

//Since malloc was used we need to free the memory when
//the object is terminated.
CharBuffer::~CharBuffer()
{
	free(m_buffer);
}

//Returns the character of the speciafied position if that position
//is withing buffer's size limits. Otherwise 0 is returned.
char CharBuffer::getChar(uint16_t index)
{
	//If out of bounds, return 0
	if (index > m_buffer_size)
	{
		return 0;
	}
	//Else return the character
	return *(m_buffer + index);
}

//Returns an integer of the item in the specified position. If the position
//is outsize buffer's size limits, -1 is returned.
int8_t CharBuffer::getInt(uint16_t index)
{
	//If out of bounds, return -1
	if (index > m_buffer_size)
	{
		return -1;
	}
	//Subtracting 0 from the char converts it to number
	return *(m_buffer + index) - '0';
}

//If the position is within limits, it sets that memory cell to
//the specified character and true is returned. If the position
//is out of limits false is returned.
bool CharBuffer::setChar(uint16_t index, char character)
{
	//If out of bounds, return false
	if (index > m_buffer_size)
	{
		return false;
	}
	//Replace character
	*(m_buffer + index) = character;
	return true;
}

//Compares the word with the first x letters of the buffer,
//where x is the size of the word. If the size of the word is within
//limits, the comparison takes place and depending on the outcome,
//true is returned for a match and false in any other case.
bool CharBuffer::find(const char *word)
{
	//A larger number size is used for string length to avoid
	//number overflows.
	uint16_t word_length = strlen(word);
	//If the word length is larger than the buffer, return false.
	if (word_length > m_buffer_size)
	{
		return false;
	}
	//A comparison takes place and true or false is returned
	//depending on the outcome.
	if (strncmp(m_buffer, word, word_length) == 0)
	{
		return true;
	}
	return false;
}

//Fills the buffer's addresses with zeroes.
void CharBuffer::clear()
{
	for (uint8_t i = 0; i < m_buffer_size; i++)
	{
		m_buffer[i] = 0;
	}
}

//Converts buffer to the infamous arduino String.
String CharBuffer::toString()
{
	return String(m_buffer);
}