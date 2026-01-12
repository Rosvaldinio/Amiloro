#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>

//**********
//STRUCTURES
//**********

//this structure stores the path to
//configuration files and the assosiated
//beginings of the lines that has the
//hexcodes
typedef struct file
{
	char *path;
	char **lines;
	int line_num;
} file;

//this structure stores all the basic info
//for animating heu changes and the configuration
//files that are to be animated
typedef struct config
{
	int cycle;
	int update;
	int file_num;
	uint8_t saturation;
	uint8_t value;
	file *files;
} config;

//*********************
//FUNCTION DECLARATIONS
//*********************

//turns hsv values to rgb
//arguments: h->heu 0->360, s->saturation 0->100, v->value 0->100
//returns a pointer that points to a three element array
//with each element being a byte long
//this array represents the rgb chanels going from
//0 -> 255 
uint8_t *get_rgb(uint16_t h, uint8_t s, uint8_t v);

//converts an unsigned 8 bit integer
//to ascii charecter that reprisents
//a singal hexadecimal digit 0->F
//arguments val->values from 0 to 15
char int_to_hex(uint8_t val);

//turns rgb to hexcode
//arguments *rgb->pointer to an array for the
//rgb values
//returns a color hexcode as a string
char *get_hex(uint8_t *rgb);

//a generalised function
//that calls all aproprite
//functions to convert
//hsv values to hexcodes
//returns the string
//that stores the color value
char *hsv_to_hex(uint16_t h, uint8_t s, uint8_t v);

//converts hsv values to a number
//between 0->255
//arguments point->number that reprisents the
//region where the color chanel is the strongest
//it's a range between point+-60 and the point it self
//can be 0->360, h->hue, s->saturation, v->value
uint8_t hsv_to_rgb(uint16_t point, uint16_t h, uint8_t s, uint8_t v);

//returns the files cursor
//where the hexcode should
//be inserted
int position(char *line, FILE *fd);

//takes a pure stringified number
//and extracts the integer
//only works for unsigned integers
int to_number(char *val);

//returns the path to the main config file
char *get_config_dir();

//*****************
//THE MAIN FUNCTION
//*****************

int main(int argc, char* argv[])
{
	char *config_dir = get_config_dir();
	
	//opens the configuration file
	FILE *conf_file = fopen(config_dir, "r");
	free(config_dir);
	
	if (!conf_file)
	{
		perror("unable to open the config file, sorry ;p");
		return 3;
	}
	
	//initializes the configuration
	//and allocates memory for the
	//animated files to be stored in
	config configuration;
	configuration.file_num = 0;
	configuration.files = (file*)malloc(sizeof(file) * 5);
	for (int i = 0; i < 5; i++)
	{	
		configuration.files[i].line_num = 0;
		configuration.files[i].path = (char*)malloc(sizeof(char) * 100);
		configuration.files[i].lines = (char**)malloc(sizeof(char*) * 3);
		for (int j = 0; j < 3; j++)
		{
			configuration.files[i].lines[j] = (char*)malloc(sizeof(char) * 100);
		}
	}
	
	//reads the config file and stores the
	//values of variables in the configuration
	//object
	char line[150];
	int file_index = 0, line_index = 0;
	while(fgets(line, 150, conf_file))
	{
		if (line[0] == '#' || line[0] == '\n')
		{
			continue;
		}
		
		char formated_line[150], *keyword = NULL, *value = NULL;
		int count = 0;
		for (int i = 0; i < strlen(line); i++)
		{
			if (line[i] >= 'a' && line[i] <= 'z' && !keyword)
			{
				keyword = &formated_line[count];
			}
			else if (i > 0 && line[i - 1] == ' ' && keyword && !value)
			{
				value = &formated_line[count];
			}
			
			if (line[i] != ' ' || line[i] != '\n')
			{	
				if (line[i] == '\"')
				{	
					value = &formated_line[count];
					while(line[++i] != '\"')
					{
						formated_line[count++] = line[i];
					}
					i++;
					formated_line[count++] = '\0';
					continue;
				}
				formated_line[count++] = line[i];
				if (i < strlen(line) && line[i + 1] == ' ' || line[i + 1] == '\n')
				{
					formated_line[count++] = '\0';
				}
			}
		}
		
		if (!keyword || !value)
		{
			perror("no valid value or keyword, sorry ;p\n");
			return 4;
		}
		
		
		if(strcmp(keyword, "update") == 0)
		{
			configuration.update = to_number(value);
		}
		else if(strcmp(keyword, "cycle") == 0)
		{
			configuration.cycle = to_number(value);
		}
		else if(strcmp(keyword, "saturation") == 0)
		{
			configuration.saturation = (uint8_t)to_number(value);
		}
		else if(strcmp(keyword, "value") == 0)
		{
			configuration.value = (uint8_t)to_number(value);
		}
		else if(strcmp(keyword, "at") == 0)
		{
			strcpy(configuration.files[file_index++].path, value);
			configuration.file_num = file_index;
			line_index = 0;
		}
		else if(strcmp(keyword, "line") == 0)
		{
			strcpy(configuration.files[file_index - 1].lines[line_index++], value);
			configuration.files[file_index - 1].line_num = line_index;
		}
		else
		{
			perror("unrecognized keyword, sorry ;p\n");
			return 5;
		}
	}
	fclose(conf_file);
	
	//animates the hexcode changes
	//and writes to the animated
	//configuration files
	uint16_t h;
	char *hex;
	while (1)
	{
		time_t t = time(NULL);
		struct tm *tmp = gmtime(&t);
		int time = tmp->tm_sec;
		int mod = time % configuration.cycle;
		float prop = (float)mod/(float)configuration.cycle;
		
		h = (uint16_t)(360.0 * prop);
		hex = hsv_to_hex(h, configuration.saturation, configuration.value);
		
		for (int i = 0; i < configuration.file_num; i++){
			FILE *file_conf = fopen(configuration.files[i].path, "r+");
			
			if (!file_conf)
			{
				perror("couldn't open file, sorry ;p\n");
				printf("%s\n", configuration.files[i].path);
				return 1;
			}
			
			for (int j = 0; j < configuration.files[i].line_num; j++)
			{
				int pos = position(configuration.files[i].lines[j], file_conf);
				
				if (pos == -1)
				{
					perror("line doesn't exsist, sorry ;p\n");
					return 2;
				}
				
				fseek(file_conf, pos, SEEK_SET);
				fwrite(hex, 1, strlen(hex), file_conf);
			}
			
			fclose(file_conf);
		}
		free(hex);
		sleep(configuration.update);
	}
	
	//dealocates the allocated memory
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			free(configuration.files[i].lines[j]);
		}
		free(configuration.files[i].path);
		free(configuration.files[i].lines);
	}
	free(configuration.files);
	
	return 0;
}

char *hsv_to_hex(uint16_t h, uint8_t s, uint8_t v)
{
	uint8_t *rgb = get_rgb(h, s, v);
	return get_hex(rgb);
}

uint8_t *get_rgb(uint16_t h, uint8_t s, uint8_t v)
{
	uint8_t *rgb = (uint8_t*)malloc(sizeof(uint8_t) * 3);
	
	rgb[0] = hsv_to_rgb((h < 180)? 0 : 360, h, s, v);
	rgb[1] = hsv_to_rgb(120, h, s, v);
	rgb[2] = hsv_to_rgb(240, h, s, v);
	
	return rgb;
}

char int_to_hex(uint8_t val)
{
	if (val >= 0 && val <= 9)
	{
		return val + '0';
	}
	else
	{
		return val + 'a' - 10;
	}
}

char *get_hex(uint8_t *rgb)
{
	char *hex = (char*)malloc(7);
	
	for (int i = 0; i < 6; i++)
	{
		hex[i] = int_to_hex((i%2 == 0)? rgb[i/2]/16 : rgb[i/2]%16);
	}
	hex[6] = '\0';
	
	free(rgb);
	return hex;
}

uint8_t hsv_to_rgb(uint16_t point, uint16_t h, uint8_t s, uint8_t v)
{
	uint8_t minimus = (uint8_t)((float)(100 - s)/100.0 * 255.0);
	uint8_t maximus = (uint8_t)((float)v/100.0 * 255.0);
	
	float multiplier = (float)(maximus - minimus)/60.0;
	float body = multiplier * ((float)h-(float)point);
	int16_t result = (int16_t)(-((body < 0.0)? body * -1.0: body) + (float)(2 * maximus) - (float)minimus);
	
	if (minimus > result)
	{
		result = minimus;
	}
	else if (maximus < result)
	{
		result = maximus;
	}
	
	return (uint8_t)result;
}

int position(char *line, FILE *fd)
{
	int ind = 0;
	for (char c = fgetc(fd); c != EOF; c = fgetc(fd))
	{
		while (c == line[ind])
		{
			c = fgetc(fd);
			ind++;
		}
		if (ind == strlen(line))
		{
			fseek(fd, -1, SEEK_CUR);
			return ftell(fd);
		}
		else
		{
			ind = 0;
		}
	}
	return -1;
}

int to_number(char *val)
{
	int result = 0;
	for (int i = 0; val[i] != '\0'; i++)
	{
		if (val[i] < '0' || val[i] > '9')
		{
			perror("string not made from numerals, sorry ;p\n");
			exit(6);
		}
		result = (result * 10) + (val[i] - '0');
	}
	return result;
}

char *get_config_dir()
{
	char *result = (char*)malloc(sizeof(char) * 200);
	char *path = getenv("XDG_CONFIG_HOME");
	if (!path)
	{
		path = getenv("HOME");
		if (!path)
		{
			perror("unable to find the config file, sorry ;p\n");
			exit(7);
		}
		else
		{
			strcpy(result, path);
			strcat(result, "/.config/amiloro/amiloro.conf");
		}
	}
	else
	{
		strcpy(result, path);
		strcat(result, "/amiloro/amiloro.conf");
	}
	
	return result;
}
