
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <bitset>
#include <cctype>
#include "stb_image.h"
#include "stb_image_write.h"
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <vector>

using namespace std;


// This function embeds a message into an image using LSB method
// The input arguments are the paths of the input image, the message to embed,
// and the output image
void embedmessage(string imagepath, string message, string outputpath) {
    // Load the input image and get its width, height, and number of color
    // channels
    int width, height, channels;
    unsigned char *image =
        stbi_load(imagepath.c_str(), &width, &height, &channels, 0);

    // Check if the image was loaded successfully
    if (!image) {
        std::cout << "Error loading image " << imagepath << std::endl;
        return;
    }

    // Check if the image has at least 3 color channels (RGB)
    if (channels < 3) {
        std::cout << "Image must have at least 3 channels" << std::endl;
        stbi_image_free(image);
        return;
    }

    // Calculate the length of the message
    int message_length = message.length();

    // Check if the message can be embedded into the image
    if (message_length * 8 > width * height * 3) {
        std::cout << "Message too large to hide message in image" << std::endl;
        stbi_image_free(image);
        return;
    }

    // Loop through all the pixels in the image and embed the message
    int index = 0;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            // Check if the entire message has been embedded
            if (index >= message_length * 8) {
                break;
            }

            // Get a pointer to the current pixel
            unsigned char *pixel = image + (i * width + j) * channels;

            // Embed the message in the least significant bit of each color channel
            for (int k = 0; k < 3; k++) {
                // Check if the entire message has been embedded
                if (index >= message_length * 8) {
                    break;
                }

                // Get the next bit of the message
                int bit = (message[index / 8] >> (index % 8)) & 1;

                // Embed the bit in the least significant bit of the current color
                // channel
                pixel[k] = (pixel[k] & ~1) | bit;

                // Increment the message index
                index++;
            }
        }
    }

    // Write the output image to disk
    int result = stbi_write_png(outputpath.c_str(), width, height, channels,
                                image, width * channels);
    if (!result) {
        std::cout << "Error writing output image " << outputpath << std::endl;
    }

    // Free the memory used by the input image
    stbi_image_free(image);
}

// This function extracts a hidden message from an image file specified by
// imagepath. It returns the extracted message as a string.
string extractmessage(string imagepath) {
    int width, height, channels;
    // Load the image file specified by imagepath using stbi_load() function and
    // store the image data in the "image" variable.
    unsigned char *image =
        stbi_load(imagepath.c_str(), &width, &height, &channels, 0);
    // If the image fails to load, print an error message, free the image memory,
    // and return NULL.
    if (!image) {
        cout << "Error loading image : " << imagepath << endl;
        return NULL;
    }

    // Check if the image has at least 3 channels (RGB).
    if (channels < 3) {
        // If the image has less than 3 channels, print an error message, free the
        // image memory, and return NULL.
        cout << "Image must have at least 3 channels" << endl;
        stbi_image_free(image);
        return NULL;
    }

    // Initialize an empty character array "message" of size 1024 to store the
    // extracted message.
    char message[1024] = {0};

    // Initialize the index to zero to keep track of the message bit being
    // extracted.
    int index_ = 0;
    // Iterate through each pixel of the image and extract the least significant
    // bit of each color channel (RGB).
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (index_ >= 1024 * 8) {
                break;
            }

            unsigned char *pixel = image + (i * width + j) * channels;
            for (int k = 0; k < 3; k++) {
                if (index_ >= 1024 * 8) {
                    break;
                }

                // Extract the least significant bit of the current color channel (RGB)
                // and store it in "bit".
                int bit = pixel[k] & 1;
                // Store the extracted bit in the "message" character array.
                message[index_ / 8] |= (bit << (index_ % 8));
                // Increment the index to move to the next bit of the message.
                index_++;
            }
        }
    }

    // Free the image memory.
    stbi_image_free(image);

    // Return the extracted message as a string.
    return message;
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        cout << "Usage: stegatool createwm -inputdir INPUT-FILES-DIR -peoplelist "
             "PEOPLE-LIST-FILE -outputdir OUTPUT-FILES-DIR"
             << endl;
        cout << "       stegatool verifywm -inputdir INPUT-FILES-DIR" << endl;
        return 1;
    }

    string command = argv[1];
    string input_dir;

    if (command == "createwm") {
        if (argc != 8) {
            cout << "Usage: stegatool createwm -inputdir INPUT-FILES-DIR -peoplelist "
                 "PEOPLE-LIST-FILE -outputdir OUTPUT-FILES-DIR"
                 << endl;
            return 1;
        }

        string people_list_file, output_dir;

        for (int i = 2; i < argc; i += 2) {
            string arg = argv[i];
            if (arg == "-inputdir") {
                input_dir = argv[i + 1];
            } else if (arg == "-peoplelist") {
                people_list_file = argv[i + 1];
            } else if (arg == "-outputdir") {
                output_dir = argv[i + 1];
            } else {
                cout << "Invalid argument: " << arg << endl;
                return 1;
            }
        }

        // Open the input directory
        DIR *dir = opendir(input_dir.c_str());
        if (dir == nullptr) {
            cout << "Error: could not open input directory" << endl;
            return -1;
        }

        // Create the output directory
        if (mkdir(output_dir.c_str(), 0777) == -1) {
            cout << "Error: could not create output directory" << endl;
            closedir(dir);
            return -1;
        }

        // read the people list
        ifstream file(people_list_file);
        if (!file.is_open()) {
            cout << "Could not open people list file " << endl;
            return 1;
        }

        vector<string> messages;
        string line;
        while (getline(file, line)) {
            messages.push_back(line);
        }

        file.close();

        // Process each file in the input directory
        dirent *entry;
        int index = 0;
        while ((entry = readdir(dir)) != nullptr) {
            // Check if the file is a JPG or PNG image
            string filename = entry->d_name;
            string extension = filename.substr(filename.find_last_of(".") + 1);

            if (extension != "jpg" && extension != "png") {
                continue;
            }

            // Load the input image
            string inputPath = input_dir + "/" + filename;

            // Embed the message in the input image
            string line;
            int idx = 0;
            for (auto message : messages) {
                message.append("\0");
                string outputFilename =
                    to_string(idx) + to_string(index) + "_" + filename;
                string outputPath = output_dir + "/" + outputFilename;
                embedmessage(inputPath, message, outputPath);
                cout << "Embedded message in " << outputFilename << endl;
                idx++;
            }
            file.seekg(0);

            // Increment the index
            index++;
        }

        file.close();

        // Clean up
        closedir(dir);

        return 0;
    }

    else if (command == "verifywm") {
        if (argc != 4) {
            cout << "Usage: stegatool verifywm -inputdir INPUT-FILES-DIR" << endl;
            return 1;
        }

        input_dir = argv[3];

        // Open the directory
        DIR *dir = opendir(input_dir.c_str());
        if (dir == nullptr) {
            cerr << "Error: could not open directory" << endl;
            return 1;
        }

        // Read the directory entries
        dirent *entry;
        while ((entry = readdir(dir)) != nullptr) {
            string filename = entry->d_name;
            string extension = filename.substr(filename.find_last_of(".") + 1);
            if (extension != "jpg" && extension != "png") {
                continue;
            }

            string message_tmp = extractmessage(input_dir + "/" + entry->d_name);
            string message = "";
            for (auto c : message_tmp) {
                if (isalnum(c) || isspace(c))
                    message.push_back(c);
                else
                    break;
            }
            if (message.length() > 0) {
                cout << entry->d_name << " -> " << message << endl;
            } else {
                cout << entry->d_name << " -> No Watermark Detected!" << endl;
            }
        }

        // Close the directory
        closedir(dir);
    } else {
        cout << "Invalid command: " << command << endl;
        return 1;
    }

    return 0;
}
