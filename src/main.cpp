#include <chrono>
#include <thread>
#include <Windows.h>
#include <vector>
#include <iostream>
#include <string>
#include <random>

using namespace std;

// drawing
const unsigned FPS = 24;
std::vector<char> frame_data;

// get the initial console buffer
auto first_buffer = GetStdHandle(STD_OUTPUT_HANDLE);

// create an additional buffer for switching
auto second_buffer = CreateConsoleScreenBuffer(
    GENERIC_READ | GENERIC_WRITE,
    FILE_SHARE_WRITE | FILE_SHARE_READ,
    nullptr,
    CONSOLE_TEXTMODE_BUFFER,
    nullptr);

// assign switchable back buffer
HANDLE back_buffer = second_buffer;
bool buffer_switch = true;

// returns current window size in rows and columns
COORD get_screen_size()
{
    CONSOLE_SCREEN_BUFFER_INFO buffer_info;
    GetConsoleScreenBufferInfo(first_buffer, &buffer_info);
    const auto new_screen_width = buffer_info.srWindow.Right - buffer_info.srWindow.Left + 1;
    const auto new_screen_height = buffer_info.srWindow.Bottom - buffer_info.srWindow.Top + 1;

    return COORD{ static_cast<short>(new_screen_width), static_cast<short>(new_screen_height) };
}

// switches back buffer as active
void swapBuffers()
{
    WriteConsole(back_buffer, &frame_data.front(), static_cast<short>(frame_data.size()), nullptr, nullptr);
    SetConsoleActiveScreenBuffer(back_buffer);
    back_buffer = buffer_switch ? first_buffer : second_buffer;
    buffer_switch = !buffer_switch;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000 / FPS));
}

// input
std::string input_text;
size_t cursor_position = 0; // Track where the cursor is in the input text

void handle_input()
{
    INPUT_RECORD inputRecords[128];
    DWORD eventsRead;
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);

    ReadConsoleInput(hInput, inputRecords, 128, &eventsRead);

    for (DWORD i = 0; i < eventsRead; ++i)
    {
        if (inputRecords[i].EventType == KEY_EVENT)
        {
            KEY_EVENT_RECORD keyEvent = inputRecords[i].Event.KeyEvent;
            if (keyEvent.bKeyDown)
            {
                switch (keyEvent.wVirtualKeyCode)
                {
                case VK_LEFT:
                    if (cursor_position > 0) cursor_position--;
                    break;
                case VK_RIGHT:
                    if (cursor_position < input_text.length()) cursor_position++;
                    break;
                case VK_BACK:
                    if (!input_text.empty() && cursor_position > 0)
                    {
                        input_text.erase(cursor_position - 1, 1);
                        cursor_position--;
                    }
                    break;
                case VK_DELETE:
                    if (!input_text.empty() && cursor_position > 0)
                    {
                        input_text.erase(cursor_position, 1);
                    }
                    break;
                case VK_RETURN:
                    input_text = "";
                    cursor_position = 0;
                    break;
                default:
                    if (keyEvent.uChar.UnicodeChar >= 32 && keyEvent.uChar.UnicodeChar <= 126)
                    {
                        input_text.insert(cursor_position, 1, keyEvent.uChar.UnicodeChar);
                        cursor_position++;
                    }
                    break;
                }
            }
        }
    }
}

// ----------------------------
//       cursor visuals
// ----------------------------

bool show_cursor = true;
std::chrono::milliseconds cursor_interval(500);
auto last_toggle = std::chrono::steady_clock::now();

COORD screen_size;

void draw_char(int x, int y, char c)
{
    y++;
    frame_data[y * screen_size.X + x] = c;
}

// Gauge
// +----------------------------+
// | Rocket status      (500ms) |
// | Data age:          120ms   |
// | Parachute:         closed  |
// | Stage Lock:        closed  |
// |                            |
// +----------------------------+

class Gauge
{
public:
    int x, y;
    int field_cnt;
    int field_names_len;
    int field_values_len;

    string name;        // name of the gauge
    string updrate_ms;  // update rate in milliseconds

    vector<string> field_names;
    vector<string> field_values;

    Gauge(int x, int y, int field_cnt, int field_names_len, int field_values_len, string name = "Gauge", string updrate_ms = "500ms")
    {
        this->x = x;
        this->y = y;
        this->field_cnt = field_cnt;
        this->field_names_len = field_names_len;
        this->field_values_len = field_values_len;

        field_names.resize(field_cnt);
        field_values.resize(field_cnt);

        this->name = name;
        this->updrate_ms = updrate_ms;

        this->name.resize(field_names_len);
        this->updrate_ms.resize(field_values_len);

        for (auto& n : field_names)
        {
            n.resize(field_names_len);
            n[0] = ':';
        }

        for (auto& v : field_values)
        {
            v.resize(field_values_len);
        }
    }

    void set_field_name(int index, string name)
    {
        int i;
        for (i = 0; i < name.length(); i++)
        {
            field_names[index][i] = name[i];
        }
        field_names[index][i] = ':';
    }

    void update_field_value(int index, string value)
    {
        for (int i = 0; i < value.length(); i++)
        {
            field_values[index][i] = value[i];
        }
    }

    void draw_stick(int x, int y, int length)
    {
        draw_char(x, y, '+');
        for (int i = 1; i < length - 1; i++)
        {
            draw_char(x + i, y, '-');
        }
        draw_char(x + length - 1, y, '+');
    }

    void draw()
    {
        int tmp_x = this->x;
        int tmp_y = this->y;

        // using the correct total length for '-' characters
        int total_length = field_names_len + field_values_len + 2 + 2;
        int inner_length = field_names_len + field_values_len;

        draw_stick(tmp_x, tmp_y, total_length);

        tmp_y++;

        // draw gauge name and update rate
        draw_char(tmp_x, tmp_y, '|');
        draw_char(tmp_x + 1, tmp_y, ' ');

        for (int i = 0; i < field_names_len; i++)
        {
            draw_char(tmp_x + 2 + i, tmp_y, name[i]);
        }

        for (int i = 0; i < field_values_len; i++)
        {
            draw_char(tmp_x + 2 + field_names_len + i, tmp_y, updrate_ms[i]);
        }

        draw_char(tmp_x + 2 + inner_length, tmp_y, ' ');
        draw_char(tmp_x + 2 + inner_length + 1, tmp_y, '|');

        for (int field_index = 0; field_index < field_cnt; field_index++)
        {
            tmp_y++;

            // draw padding
            draw_char(tmp_x, tmp_y, '|');
            draw_char(tmp_x + 1, tmp_y, ' ');

            // draw field name
            for (int i = 0; i < field_names_len; i++)
            {
                draw_char(tmp_x + 2 + i, tmp_y, field_names[field_index][i]);
            }

            // draw field value
            for (int i = 0; i < field_values_len; i++)
            {
                draw_char(tmp_x + 2 + field_names_len + i, tmp_y, field_values[field_index][i]);
            }

            // draw padding
            draw_char(tmp_x + 2 + inner_length, tmp_y, ' ');
            draw_char(tmp_x + 2 + inner_length + 1, tmp_y, '|');
        }

        tmp_y++;
        draw_stick(tmp_x, tmp_y, total_length);
    }
};

vector<Gauge> gauges;

void init_gauges()
{
    gauges.push_back(Gauge(0, 0, 4, 16, 8, "Gauge", "500ms"));
    gauges[0].set_field_name(0, "Data age");
}

void draw_gauges()
{
    for (auto& gauge : gauges)
    {
        gauge.draw();
    }
}

class Command
{

};

class Log
{

};

// draw frame with user input text
void draw_frame(COORD screen_size)
{
    frame_data.assign(screen_size.X * screen_size.Y, ' ');

    draw_char(80, 0, 'H');

    draw_gauges();

    // Draw input text at the bottom of the console
    int input_line_index = screen_size.Y - 1;

    for (size_t i = 0; i < input_text.size() && i < screen_size.X; ++i)
    {
        frame_data[input_line_index * screen_size.X + i] = input_text[i];
    }

    // Toggle cursor visibility
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_toggle) > cursor_interval)
    {
        show_cursor = !show_cursor;
        last_toggle = now;
    }

    if (cursor_position < screen_size.X)
    {
        if (show_cursor || cursor_position >= input_text.length())
        {
            frame_data[input_line_index * screen_size.X + cursor_position] = '_';
        }
    }
}

// function that will be called from a thread
void call_from_thread()
{
    while (true)
    {
    }
}

int main()
{
    init_gauges();

    screen_size = get_screen_size();
    SetConsoleScreenBufferSize(first_buffer, screen_size);
    SetConsoleScreenBufferSize(second_buffer, screen_size);
    frame_data.resize(screen_size.X * screen_size.Y);

    random_device rd;  // Obtain a random number from hardware
    mt19937 gen(rd()); // Seed the generator
    uniform_int_distribution<> distr(0, 10);

    int random_number = distr(gen);

    std::thread t1(call_from_thread);

    // Flush the console input buffer to remove any existing inputs
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));

    // main rendering loop:
    while (true)
    {
        handle_input();
        draw_frame(screen_size);
        swapBuffers();
        gauges[0].update_field_value(0, to_string((int)(distr(gen))));
    }

    return 0;
}
