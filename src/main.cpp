#include <Windows.h>
#include <iostream>

#include <random>

#include <thread>

#include <chrono>
#include <ctime>

#include <vector>

#include <string>
#include <iomanip>
#include <sstream>

#include <unordered_map>

#include "param.h"

using namespace std;

#define PADDING_HORIZONTAL 2
#define PADDING_VERTICAL 1

// drawing
const unsigned FPS = 24;
std::vector<char> frame_data;

// time
chrono::time_point<chrono::steady_clock> now;

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
string get_current_time_string()
{
    // get the current time
    time_t now = time(0);

    // convert now to tm struct for local timezone
    tm* localtm = localtime(&now);

    // use a stringstream to format the time
    stringstream ss;
    ss << setfill('0') << setw(2) << localtm->tm_hour << ":"
        << setfill('0') << setw(2) << localtm->tm_min << ":"
        << setfill('0') << setw(2) << localtm->tm_sec;

    // return the formatted time string
    return ss.str();
}


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
void swap_buffers()
{
    WriteConsole(back_buffer, &frame_data.front(), static_cast<short>(frame_data.size()), nullptr, nullptr);
    SetConsoleActiveScreenBuffer(back_buffer);
    back_buffer = buffer_switch ? first_buffer : second_buffer;
    buffer_switch = !buffer_switch;
    this_thread::sleep_for(chrono::milliseconds(1000 / FPS));
}

uint16_t assemble_uint16_t(const std::string& str)
{
    uint8_t first_byte = static_cast<uint8_t>(str[0]);
    uint8_t second_byte = static_cast<uint8_t>(str[1]);

    // assemble into uint16_t
    uint16_t result = (static_cast<uint16_t>(first_byte) << 8) | second_byte;

    return result;
}

bool check_provider_id_validity()
{

}

bool check_command_id_validity()
{

}

bool check_crc()
{

}

class GenericProvider
{
    uint16_t provider_id;

    unordered_map<uint16_t, uint16_t> command_map;

    GenericProvider() = default;

    GenericProvider(uint16_t provider_id)
    {
        this->provider_id = provider_id;
    }
};


class GenericCommand
{
public:
    uint16_t command_id;
    uint16_t argument;

    GenericCommand() = default;

    GenericCommand(uint16_t provider_id, uint16_t command_id, uint16_t argument)
    {
        this->provider_id = provider_id;
        this->command_id = command_id;
        this->argument = argument;
    }
};

class GenericReply
{

};

unordered_map<string, GenericCommand> command_map = {
    {"iudt", GenericCommand()}
};

vector<GenericProvider> generic_providers;
vector<GenericCommand> generic_commands;
vector<GenericReply> generic_replies;

void init_generic_command()
{
    generic_commands.push_back(GenericCommand());
}

void init_generic_reply()
{

}

struct packet_parse_result
{
    uint16_t provider_id;
    uint16_t command_id;
};

enum comms_state
{
    awaiting_reply,
    idle,
};

enum packet_validation_state
{
    expecting_marker_start,
    in_packet,
    expecting_marker_end,
};

packet_parse_result pr;

comms_state cs = idle;

packet_validation_state vs = expecting_marker_start;

uint16_t packet_validation_ptr = 0;
bool start_marker_found = false;
uint16_t oldest_start_marker_ptr = 0;

uint8_t packet_data_segment_read_cnt = 0;
uint16_t packet_data_segment_start_ptr = 0;

uint16_t comms_rcv_buffer_write_ptr = 0;

vector<vector<uint16_t>> comms_snd_buffer;
char comms_rcv_buffer[COMMS_RCV_BUFFER_SIZE];

uint16_t packet_validation_ptr = 0;

inline void comms_rcv_buffer_move_ptr_right(uint16_t& ptr, uint16_t dst)
{
    ptr = (ptr + dst) % COMMS_RCV_BUFFER_SIZE;
}

inline void comms_rcv_buffer_move_ptr_left(uint16_t& ptr, uint16_t dst)
{
    // handling potential negative result by adding COMMS_RCV_BUFFER_SIZE before modulo
    ptr = (ptr + COMMS_RCV_BUFFER_SIZE - dst) % COMMS_RCV_BUFFER_SIZE;
}

inline char comms_rcv_buffer_peek_right(uint16_t ptr, uint16_t dst)
{
    return comms_rcv_buffer[(ptr + dst) % COMMS_RCV_BUFFER_SIZE];
}

inline char comms_rcv_buffer_peek_left(uint16_t ptr, uint16_t dst)
{
    return comms_rcv_buffer[(ptr + COMMS_RCV_BUFFER_SIZE - dst) % COMMS_RCV_BUFFER_SIZE];
}

bool uart_is_readable()
{
    return true;
}

char uart_getc()
{
    return 'a';
}

void comms_update()
{
    switch (cs)
    {
    case awaiting_reply:
        while (uart_is_readable())
        {
            comms_rcv_buffer[comms_rcv_buffer_write_ptr] = uart_getc();
            comms_rcv_buffer_move_ptr_right(comms_rcv_buffer_write_ptr, 1);

            for (
                packet_validation_ptr = (comms_rcv_buffer_write_ptr + COMMS_RCV_BUFFER_SIZE - 1) % COMMS_RCV_BUFFER_SIZE;
                packet_validation_ptr != comms_rcv_buffer_write_ptr;
                comms_rcv_buffer_move_ptr_right(packet_validation_ptr, 1)
                )
            {
                char current = comms_rcv_buffer[packet_validation_ptr];

                // record the oldest start marker pointer that we have found inside the packet that we are currently validating
                // because it might be the start of a new (valid) packet
                if (current == MARKER_START && !start_marker_found && vs != expecting_marker_start)
                {
                    start_marker_found = true;
                    oldest_start_marker_ptr = packet_validation_ptr;
                }

                switch (vs)
                {
                case expecting_marker_start:

                    // packet started
                    if (current == MARKER_START)
                    {
                        vs = in_packet;
                    }
                    // else the packet is bad, ignore the character
                    break;

                case in_packet:

                    // we read one character of the data segment
                    packet_data_segment_read_cnt++;

                    // if it's the first character of the data segment, record the start pointer
                    if (packet_data_segment_read_cnt == 1)
                    {
                        packet_data_segment_start_ptr = packet_validation_ptr;
                    }
                    // if it's the last character of the data segment, move to the next state
                    else if (packet_data_segment_read_cnt == PACKET_LENGTH)
                    {
                        // reset the data segment read counter for later
                        packet_data_segment_read_cnt = 0;
                        vs = expecting_marker_end;
                    }
                    break;

                case expecting_marker_end:

                    // packet is valid
                    if (current == MARKER_END)
                    {
                        ppr = parse_packet(packet_data_segment_start_ptr);
                        vs = expecting_marker_start;

                        // start execution of the command specified in parsed packet
                        // execute_command(ppr.provider_id, ppr.command_id, ppr.argument);
                    }
                    // packet ended prematurely
                    else
                    {
                        // move validation pointer to the oldest start marker
                        // because it might be the start of a new (valid) packet
                        packet_validation_ptr = oldest_start_marker_ptr;
                        vs = expecting_marker_start;
                    }

                    break;
                }
            }
        }
    case idle:
        break;
    }
}


COORD screen_size;

void draw_char(int x, int y, char c)
{
    y++;
    frame_data[y * screen_size.X + x] = c;
}



int validate_command(string command)
{

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

    int total_length;
    int inner_length;

    Gauge() = default;

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

        this->total_length = field_names_len + field_values_len + 2 + 2;
        this->inner_length = field_names_len + field_values_len;

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

    void init_draw()
    {
        int tmp_x = this->x;
        int tmp_y = this->y;

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

    void draw()
    {
        int tmp_x = this->x;
        int tmp_y = this->y;

        tmp_y++;

        for (int field_index = 0; field_index < field_cnt; field_index++)
        {
            tmp_y++;

            // draw field value
            for (int i = 0; i < field_cnt; i++)
            {
                draw_char(tmp_x + 2 + field_names_len + i, tmp_y, field_values[field_index][i]);
            }
        }
    }
};

class ScrollingText
{
public:
    int x;
    int y;

    int width;
    int height;

    vector<string> lines;

    ScrollingText() = default;

    ScrollingText(int x, int y, int width, int height, bool inverted = false)
    {
        this->x = x;
        this->y = y;
        this->width = width;
        this->height = height;
    }

    void add_line(string line)
    {
        int start_pos = 0;
        int line_length = line.length();

        // process the line until all characters are accounted for
        while (start_pos < line_length)
        {
            // find the next newline character from the current start position
            int newline_pos = line.find('\n', start_pos);

            // determine the end of the current segment to process (up to newline or end of line)
            int end_pos = (newline_pos != string::npos) ? newline_pos : line_length;

            // process the current segment
            while (start_pos < end_pos)
            {
                // calculate the remaining characters in this segment
                int remaining_chars = end_pos - start_pos;

                // determine how many characters to take for this line segment, constrained by width
                int segment_length = min(width, remaining_chars);

                // extract the substring for the current segment
                string segment = line.substr(start_pos, segment_length);

                // append the segment to the lines vector
                lines.insert(lines.begin(), segment);

                // move the start position forward by the segment length
                start_pos += segment_length;
            }

            // skip over the newline character if one was found
            if (newline_pos != string::npos && start_pos == newline_pos)
            {
                start_pos++;
            }

            // if a newline was processed, add an empty line if it's the last character
            if (start_pos == end_pos && newline_pos != string::npos)
            {
                lines.push_back("");
            }
        }
    }


    void erase()
    {
        int tmp_x = this->x;
        int tmp_y = this->y;

        for (int i = tmp_y + height - 1; i >= tmp_y; i--)
        {
            for (int j = 0; j < width; j++)
            {
                draw_char(tmp_x + j, tmp_y + i, ' ');
            }
        }
    }

    // updates and erases the previous frame at the same time
    void draw()
    {
        int tmp_x = this->x;
        int tmp_y = this->y;

        int lines_read_ptr = 0;

        for (int i = height; i >= tmp_y; i--)
        {
            if (lines_read_ptr < lines.size())
            {
                for (int j = 0; j < width; j++)
                {
                    if (j < lines[lines_read_ptr].length())
                    {
                        draw_char(tmp_x + j, tmp_y + i, lines[lines_read_ptr][j]);
                    }
                    else
                    {
                        draw_char(tmp_x + j, tmp_y + i, ' ');
                    }
                }
                lines_read_ptr++;
            }
            else
            {
                for (int j = 0; j < width; j++)
                {
                    draw_char(tmp_x + j, tmp_y + i, ' ');
                }
            }
        }
    }
};

vector<ScrollingText> scrolling_texts;

class InputField
{
public:
    int x, y;
    int width;
    chrono::milliseconds cursor_interval;


    bool show_cursor = true;
    string input_text = "";
    size_t cursor_position = 0;
    std::chrono::time_point<std::chrono::steady_clock> last_toggle;

    InputField() = default;

    InputField(int x, int y, int width, int cursor_interval = 500)
    {
        this->x = x;
        this->y = y;
        this->width = width;
        this->cursor_interval = chrono::milliseconds(cursor_interval);

        last_toggle = chrono::steady_clock::now();
    }

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
                        if (cursor_position > 0)
                        {
                            cursor_position--;
                        }
                        break;
                    case VK_RIGHT:
                        if (cursor_position < input_text.length())
                        {
                            cursor_position++;
                        }
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

                        scrolling_texts[0].add_line(get_current_time_string() + " > " + input_text);
                        scrolling_texts[0].draw();

                        input_text = "";
                        cursor_position = 0;
                        break;
                    default:
                        if (keyEvent.uChar.UnicodeChar >= 32 && keyEvent.uChar.UnicodeChar <= 126 && cursor_position < width)
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

    void erase()
    {
        int tmp_x = this->x;
        int tmp_y = this->y;

        for (size_t i = 0; i < width; ++i)
        {
            draw_char(tmp_x + i, tmp_y, ' ');
        }
    }

    void draw()
    {
        int tmp_x = this->x;
        int tmp_y = this->y;

        // erase previous frame
        erase();

        for (size_t i = 0; i < input_text.size(); ++i)
        {
            draw_char(tmp_x + i, tmp_y, input_text[i]);
        }

        // toggle cursor visibility
        auto now = chrono::steady_clock::now();
        if (chrono::duration_cast<chrono::milliseconds>(now - last_toggle) > cursor_interval)
        {
            show_cursor = !show_cursor;
            last_toggle = now;
        }

        // draw cursor
        if (cursor_position < screen_size.X)
        {
            if (cursor_position == width)
            {
                draw_char(tmp_x + cursor_position, tmp_y, ' ');
            }
            else if (show_cursor)
            {
                draw_char(tmp_x + cursor_position, tmp_y, '_');
            }
            else if (!show_cursor && cursor_position >= input_text.length())
            {
                draw_char(tmp_x + cursor_position, tmp_y, ' ');
            }
        }
    }

    void init_draw()
    {

    }
};

class CommandWindow
{
public:
    int x;
    int y;

    ScrollingText log_window;

    CommandWindow() = default;

    CommandWindow(int x, int y)
    {
        this->x = x;
        this->y = y;
    }

    void init_draw()
    {
        int tmp_x = this->x;
        int tmp_y = this->y;

        for (int i = tmp_y; i < screen_size.Y; i++)
        {
            draw_char(tmp_x, tmp_y + i, '|');
        }
    }
};


vector<Gauge> gauges;
InputField input_field;

void init_gauges()
{
    gauges.push_back(Gauge(PADDING_HORIZONTAL, PADDING_VERTICAL, 4, 16, 8, "Rocket status", "500ms"));
    gauges[0].set_field_name(0, "Data age");
    gauges[0].init_draw();

    gauges.push_back(Gauge(PADDING_HORIZONTAL + gauges[0].total_length - 1, PADDING_VERTICAL, 3, 16, 8, "Acceleration", "750ms"));
    gauges[1].set_field_name(0, "X");
    gauges[1].set_field_name(1, "Y");
    gauges[1].set_field_name(2, "Z");
    gauges[1].init_draw();

    gauges.push_back(Gauge(PADDING_HORIZONTAL + gauges[0].total_length - 1, PADDING_VERTICAL + 5, 3, 16, 8, "Gyro", "750ms"));
    gauges[2].set_field_name(0, "X");
    gauges[2].set_field_name(1, "Y");
    gauges[2].set_field_name(2, "Z");
    gauges[2].init_draw();

}

void draw_gauges()
{
    for (int i = 0; i < gauges.size(); i++)
    {
        gauges[i].draw();
    }
}

void init_command_window()
{
    CommandWindow command_window = CommandWindow(75, 1);
    command_window.init_draw();
}

void init_input_field()
{
    input_field = InputField(78, screen_size.Y - 1 - PADDING_VERTICAL, 40);
    input_field.init_draw();
}

void init_scrolling_texts()
{
    scrolling_texts.push_back(ScrollingText(78, PADDING_VERTICAL, 40, screen_size.Y - 4));
    scrolling_texts[0].add_line("This is a test line\naaaaaaaaa\naaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    scrolling_texts[0].draw();
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
    // frame_data.assign(screen_size.X * screen_size.Y, ' ');

    draw_char(75, 1, 'H');

    draw_gauges();

    input_field.draw();
}

// function that will be called from a thread
void call_from_thread()
{
    while (true)
    {
    }
}

void setup_console()
{
    COORD bufferSize = get_screen_size();
    SetConsoleScreenBufferSize(first_buffer, bufferSize);
    SetConsoleScreenBufferSize(second_buffer, bufferSize);

    // Clearing any unintended output before we start drawing
    system("cls");  // Clears the console

    // Ensuring cursor is positioned at the beginning to avoid initial prints messing up the buffer
    SetConsoleCursorPosition(first_buffer, { 0, 0 });
    SetConsoleCursorPosition(second_buffer, { 0, 0 });
}

void make_console_unresizable()
{
    HWND consoleWindow = GetConsoleWindow();  // Get the console window handle
    LONG style = GetWindowLong(consoleWindow, GWL_STYLE);  // Get current window style
    style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);  // Remove resizing and maximize capabilities
    SetWindowLong(consoleWindow, GWL_STYLE, style);  // Set the new style
    SetWindowPos(consoleWindow, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);  // Apply the new style
}

int main()
{
    make_console_unresizable();
    screen_size = get_screen_size();
    SetConsoleScreenBufferSize(first_buffer, screen_size);
    SetConsoleScreenBufferSize(second_buffer, screen_size);
    frame_data.resize(screen_size.X * screen_size.Y);

    init_gauges();
    init_input_field();
    init_command_window();
    init_scrolling_texts();

    random_device rd;  // Obtain a random number from hardware
    mt19937 gen(rd()); // Seed the generator
    uniform_int_distribution<> distr(0, 10);

    int random_number = distr(gen);

    thread t1(call_from_thread);

    // flush the console input buffer to remove any existing inputs
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));

    // main rendering loop:
    while (true)
    {
        now = chrono::steady_clock::now();
        input_field.handle_input();
        draw_frame(screen_size);
        swap_buffers();
        gauges[0].update_field_value(0, to_string((int)(distr(gen))));
    }

    return 0;
}
