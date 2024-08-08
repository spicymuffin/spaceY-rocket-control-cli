#include <chrono>
#include <thread>
#include <Windows.h>
#include <vector>
#include <iostream>

/*
Console coordinate system:

+-----------------X increasing
|
|
|
|
|
Y increasing

*/

// drawing
const unsigned FPS = 24;
std::vector<char> frame_data;

// get the intial console buffer.
auto first_buffer = GetStdHandle(STD_OUTPUT_HANDLE);

// create an additional buffer for switching.
auto second_buffer = CreateConsoleScreenBuffer(
    GENERIC_READ | GENERIC_WRITE,
    FILE_SHARE_WRITE | FILE_SHARE_READ,
    nullptr,
    CONSOLE_TEXTMODE_BUFFER,
    nullptr);

// assign switchable back buffer.
HANDLE back_buffer = second_buffer;
bool buffer_switch = true;

// returns current window size in rows and columns.
COORD get_screen_size()
{
    CONSOLE_SCREEN_BUFFER_INFO buffer_info;
    GetConsoleScreenBufferInfo(first_buffer, &buffer_info);
    const auto new_screen_width = buffer_info.srWindow.Right - buffer_info.srWindow.Left + 1;
    const auto new_screen_height = buffer_info.srWindow.Bottom - buffer_info.srWindow.Top + 1;

    return COORD{ static_cast<short>(new_screen_width), static_cast<short>(new_screen_height) };
}

// switches back buffer as active.
void swapBuffers()
{
    WriteConsole(back_buffer, &frame_data.front(), static_cast<short>(frame_data.size()), nullptr, nullptr);
    SetConsoleActiveScreenBuffer(back_buffer);
    back_buffer = buffer_switch ? first_buffer : second_buffer;
    buffer_switch = !buffer_switch;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000 / FPS));
}

short cursor = 0;

int get_line_start_index(COORD screen_size, short line)
{
    return line * screen_size.X;
}

// draw horizontal line moving from top to bottom.
void draw_frame(COORD screen_size)
{
    int line_start_index = get_line_start_index(screen_size, 20);

    for (auto i = 0; i < screen_size.Y; i++)
    {
        for (auto j = 0; j < screen_size.X; j++)
        {
            if (i == 20)
            {
                frame_data[screen_size.X * i + j] = '-';
            }
            else
            {
                frame_data[screen_size.X * i + j] = ' ';
            }
        }
    }

    // for (auto i = 0; i < screen_size.Y; i++)
    // {
    //     for (auto j = 0; j < screen_size.X; j++)
    //         if (cursor == i)
    //             frame_data[i * screen_size.X + j] = '@';
    //         else
    //             frame_data[i * screen_size.X + j] = ' ';
    // }

    // cursor++;
    // if (cursor >= screen_size.Y)
    //     cursor = 0;
}

int main()
{
    const auto screen_size = get_screen_size();
    SetConsoleScreenBufferSize(first_buffer, screen_size);
    SetConsoleScreenBufferSize(second_buffer, screen_size);
    frame_data.resize(screen_size.X * screen_size.Y);

    // main rendering loop:
    // 1. draw frame to the back buffer
    // 2. set back buffer as active

    while (true)
    {
        draw_frame(screen_size);
        swapBuffers();
    }
}

// int main() {
//     HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
//     INPUT_RECORD inputRecord;
//     DWORD eventsRead;

//     // Set up the console to receive input events
//     while (TRUE) {
//         // Read input events one by one
//         ReadConsoleInput(hInput, &inputRecord, 1, &eventsRead);

//         if (inputRecord.EventType == KEY_EVENT) {
//             KEY_EVENT_RECORD keyEvent = inputRecord.Event.KeyEvent;

//             // Check if the key is pressed down
//             if (keyEvent.bKeyDown) {
//                 printf("Key code: %u\n", keyEvent.wVirtualKeyCode);

//                 // Exit loop if 'ESC' key is pressed
//                 if (keyEvent.wVirtualKeyCode == VK_ESCAPE) {
//                     break;
//                 }
//             }
//         }
//     }

//     return 0;
// }