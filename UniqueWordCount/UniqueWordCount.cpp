// UniqueWordCount.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <set>
#include <vector>
#include <functional>
#include <thread> 

using namespace std;

constexpr int THREAD_COUNT = 5;
constexpr int BUFFER_LENGTH = 50;
constexpr char WORD_SEP = ' ';

void ProcessBuffer(char* buffer, int nStart, int nEnd, std::function<void(char*, int, std::set<std::vector<char>>&)> wordHandler, std::set<std::vector<char>>& unique_words)
{
	int startPos = nStart;

	for (int i = nStart; i < nEnd; ++i)
	{
		if (buffer[i] == WORD_SEP)
		{
			wordHandler(buffer + startPos, i - startPos, unique_words);
			startPos = i + 1;
		}
	}

	// handle the last word
	wordHandler(buffer + startPos, nEnd - startPos, unique_words);
}

size_t UniqueWordsCount(ifstream& file, std::function<void(char*, int, std::set<std::vector<char>>&)> wordHandler)
{
	std::set<std::vector<char>> main_unique_words;

	file.seekg(0, ios::end);
	const long long fileSize = file.tellg();
	file.seekg(0, ios::beg);

	std::vector<char*> buffers;
	buffers.reserve(THREAD_COUNT);

	int bufferLength = BUFFER_LENGTH;
	int bufferCountForThread = bufferLength / THREAD_COUNT;
	buffers.push_back(new char[bufferLength]);

	// generate threads to process buffer
	std::vector<std::thread> threads;
	threads.reserve(THREAD_COUNT);
	for (int i = 0; i < THREAD_COUNT; i++)
		threads.emplace_back(std::thread());

	// each thread has to has a unique set with words that will be merged
	std::vector<std::set<std::vector<char>>> unique_words;
	for (int i = 0; i < THREAD_COUNT; i++)
		unique_words.emplace_back(std::set<std::vector<char>>());

	int readSize = bufferLength;
	int bufferLengthToHandle = bufferLength;
	file.read(buffers[0], readSize);
	long long bufferPosInFile = 0;
	int bufferNumToProcess = 0;

	while (fileSize > bufferPosInFile)
	{
		bufferPosInFile += readSize;

		// shift the buffer to the first separator from the end
		if (fileSize > bufferPosInFile)
		{
			while (buffers[bufferNumToProcess][bufferLengthToHandle] != WORD_SEP && bufferLengthToHandle > -1)
				--bufferLengthToHandle;
		}

		// check if separator was found to process the buffer
		if (bufferLengthToHandle > -1)
		{
			int nStart = 0;
			int nEnd = bufferCountForThread;

			for (int i = 0; i < THREAD_COUNT; ++i)
			{
				while (nEnd < bufferLengthToHandle && buffers[bufferNumToProcess][nEnd] != WORD_SEP)
					++nEnd;

				if (threads[i].joinable())
				{
					threads[i].join();
					main_unique_words.insert(unique_words[i].begin(), unique_words[i].end());
				}

				unique_words[i].clear();
				threads[i] = std::thread(ProcessBuffer, buffers[bufferNumToProcess], nStart, std::min(nEnd + 1, bufferLengthToHandle + 1), wordHandler, std::ref(unique_words[i]));

				nStart = std::min(nEnd + 1, bufferLengthToHandle + 1);
				nEnd = nStart + bufferCountForThread;
			}
		}

		// read new buffer and keep text that was not handled in the buffer
		int lengthToMove = bufferLength - bufferLengthToHandle - 1;
		if (lengthToMove >= 0)
		{
			// increase the buffer if the word could be placed inside available buffer
			if (bufferLengthToHandle < 0)
			{
				bufferLength += BUFFER_LENGTH;
				bufferCountForThread = bufferLength / THREAD_COUNT;
				bufferLengthToHandle = bufferLength;

				char* tmpBuffer = new char[bufferLength];
				memmove(tmpBuffer, buffers[bufferNumToProcess], bufferLength);

				delete[] buffers[bufferNumToProcess];
				buffers[bufferNumToProcess] = tmpBuffer;
			}
			else
			{
				// move the data that was not process into new buffer for the thread
				buffers.push_back(new char[bufferLength]);
				memmove(buffers.back(), buffers[bufferNumToProcess] + bufferLengthToHandle + 1, lengthToMove);
				++bufferNumToProcess;
			}

			// readSize - read data up to the bufferLength
			readSize = bufferLength - lengthToMove;
			if (file && readSize != 0)
				file.read(buffers.back() + lengthToMove, readSize);

			// shift buffer to the available data
			if (readSize + bufferPosInFile - lengthToMove > fileSize)
			{
				bufferLengthToHandle = static_cast<int>(fileSize - bufferPosInFile + lengthToMove);

				char* tmpBuffer = new char[bufferLengthToHandle];
				memmove(tmpBuffer, buffers[bufferNumToProcess], bufferLengthToHandle);

				readSize = bufferLengthToHandle;
				bufferLength = bufferLengthToHandle;

				delete[] buffers[bufferNumToProcess];
				buffers[bufferNumToProcess] = tmpBuffer;
			}
		}
	}

	for (int i = 0; i < THREAD_COUNT; ++i)
	{
		if (threads[i].joinable())
			threads[i].join();
		main_unique_words.insert(unique_words[i].begin(), unique_words[i].end());
	}

	for (auto& buffer : buffers)
		delete[] buffer;

	return main_unique_words.size();
}

int main()
{
	ifstream file;
	file.open("12345.txt");

	// function to insert the word into the container
	std::function<void(char*, int length, std::set<std::vector<char>>&)> handler = [](char* buffer, int length, std::set<std::vector<char>>& unique_words) -> void
	{
		if (!buffer || length <= 0)
			return;

		unique_words.insert(std::vector<char>(buffer, buffer + length));
	};

	cout << "Unique words count:" << UniqueWordsCount(file, handler);
	//std::string word;
	//std::set<std::string> unique_words;
	//while(file >>word)
	//{
	//	unique_words.insert(word);
	//}
	//cout << "Unique words count:" << unique_words.size();
}
