#ifndef TIME_O_CLOCK_MESSAGES_H
#define TIME_O_CLOCK_MESSAGES_H


typedef struct{
	const char* msg;
	const int weight;
} Message;


const Message messages[] = {
	{
		.msg = "Hey guys! It's time O'clock!",
		.weight = 100
	},
	{
		.msg = "It's time O'clock! Time for a shit",
		.weight = 5
	},
	{
		.msg = "Oh fuck it's time O'clock!",
		.weight = 1
	}
};

const int messages_count = 3;

#endif //TIME_O_CLOCK_MESSAGES_H
