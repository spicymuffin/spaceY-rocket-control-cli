#include <chrono>
#include <thread>
#include <Windows.h>
#include <vector>
#include <iostream>
#include <string>

// drawing
const unsigned FPS = 24;
std::vector<char> frame_data;
std::string input_text;
size_t cursor_position = 0; // Track where the cursor is in the input text

// get the initial console buffer.
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

void handle_input()
{
    INPUT_RECORD inputRecords[128];
    DWORD eventsRead;
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);

    ReadConsoleInput(hInput, inputRecords, 128, &eventsRead);

    for (DWORD i = 0; i < eventsRead; ++i) {
        if (inputRecords[i].EventType == KEY_EVENT) {
            KEY_EVENT_RECORD keyEvent = inputRecords[i].Event.KeyEvent;
            if (keyEvent.bKeyDown) {
                switch (keyEvent.wVirtualKeyCode) {
                    case VK_LEFT:
                        if (cursor_position > 0) cursor_position--;
                        break;
                    case VK_RIGHT:
                        if (cursor_position < input_text.length()) cursor_position++;
                        break;
                    case VK_BACK:
                        if (!input_text.empty() && cursor_position > 0) {
                            input_text.erase(cursor_position - 1, 1);
                            cursor_position--;
                        }
                        break;
                    default:
                        if (keyEvent.uChar.UnicodeChar >= 32 && keyEvent.uChar.UnicodeChar <= 126) {
                            input_text.insert(cursor_position, 1, keyEvent.uChar.UnicodeChar);
                            cursor_position++;
                        }
                        break;
                }
            }
        }
    }
}

// draw frame with user input text.
void draw_frame(COORD screen_size)
{
    frame_data.assign(screen_size.X * screen_size.Y, ' ');

    // Draw input text at the bottom of the console
    int input_line_index = screen_size.Y - 1;
    for (size_t i = 0; i < input_text.size() && i < screen_size.X; ++i) {
        frame_data[input_line_index * screen_size.X + i] = input_text[i];
    }

    // Display cursor
    if (cursor_position < screen_size.X) {
        frame_data[input_line_index * screen_size.X + cursor_position] = '|';
    }
}

int main()
{
    const auto screen_size = get_screen_size();
    SetConsoleScreenBufferSize(first_buffer, screen_size);
    SetConsoleScreenBufferSize(second_buffer, screen_size);
    frame_data.resize(screen_size.X * screen_size.Y);

    // Flush the console input buffer to remove any existing inputs
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));

    // main rendering loop:
    while (true)
    {
        handle_input();
        draw_frame(screen_size);
        swapBuffers();
    }

    return 0;
}