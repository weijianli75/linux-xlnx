/*
 * P6 Power Upgrade Routines
 */

/*
 * Upgrade operations
 */
#define UPGRADE_INIT_ID			0x5555AAAA
#define UPGRADE_ID				0xAAAA5555

#define SEND_PACKET_ID			0x18FF58E6
#define REPLY_PACKET_ID			0x18FF58E7

#define DEFAULT_UPGRADE_RETRIES	1000

#define COMMAND_PACKET_BUFFER_SIZE	32
#define REPLY_PACKET_BUFFER_SIZE	96

#define UPGRADE_DATA_BUFFER_SIZE	(1 * 1024 * 1024) /* 1 MB */

static char *hex_data_buf;
static unsigned int hex_data_size;
static unsigned int next_data_pos = 0;
static unsigned int data_high_addr = 0, data_low_addr = 0;
static char upgrade_status[256];

static int upgrade_retries;
static int upgrade_started;
static int upgrade_finished;
static int upgrade_success_count;
static int upgrade_cmd_retry_count;
static int upgrade_data_retry_count;
static int upgrade_fail_count;

static int send_data_to_power(struct p6_data *pdata, u32 command, u32 data,
                              u32 reply_command, u32 reply_data, int retries_count);

static void init_upgrade_state(void)
{
    memset(hex_data_buf, 0, UPGRADE_DATA_BUFFER_SIZE);
    hex_data_size = 0;
    upgrade_retries = DEFAULT_UPGRADE_RETRIES;
    upgrade_started = 0;
    upgrade_finished = 0;
    upgrade_success_count = 0;
    upgrade_cmd_retry_count = 0;
    upgrade_data_retry_count = 0;
    upgrade_fail_count = 0;
}

static void update_upgrade_status(unsigned int command, unsigned int data, int pass)
{
    memset(upgrade_status, 0, sizeof(upgrade_status));
    sprintf(upgrade_status, "start:%d cmd:%08x data:%08x pass:%d finish:%d success_count:%d retry_count:%d:%d fail_count:%d",
            upgrade_started, command, data, pass, upgrade_finished, upgrade_success_count, upgrade_cmd_retry_count, upgrade_data_retry_count, upgrade_fail_count);
}

//将0-F字符的ascii码转换成十六进制数
//若输入字符并非0-9，A-F则返回0xff，以表示故障
//输入a-f小写形式也会报错
static unsigned char ascii_to_hex(char ascii_code)
{
    int temp;
    int ascii_char;
    ascii_char = (int)ascii_code;

    if ((ascii_char >= 0x30) && (ascii_char <= 0x39)) {
        temp = ascii_char - 0x30; // 接收到0-9字符
    }
    else {
        if ((ascii_char >= 0x41) && (ascii_char <= 0x46)) {
            temp = ascii_char - 0x37; //接收到A-F字符，A（0x41,65） F（0x46,70）0x37=55
        }
        else {
            temp = 0xff; //若接收到
        }
    }
    return (unsigned char)temp;
}

//返回值为本行的字节数
//专用函数
static int word_count_analysis(char high_char, char low_char)
{
    int temp;
    char high_byte;
    char low_byte;
    temp = 0;
    high_byte = ascii_to_hex(high_char);
    high_byte &= 0xf;
    low_byte = ascii_to_hex(low_char);
    low_byte &= 0xf;
    temp = (high_byte << 4) + low_byte;
    temp = temp >> 1; //hex文件中的是数据的byte数，而我们要的是数据的word字数（16bit）,所以除以2
    return temp;
}

static unsigned short combine_byte_into_word(unsigned char byte3, unsigned char byte2, unsigned char byte1, unsigned char byte0)
{
    unsigned short temp;
    byte0 &= 0xf;
    byte1 &= 0xf;
    byte2 &= 0xf;
    byte3 &= 0xf;
    temp = (unsigned short)((byte3 << 12) + (byte2 << 8) + (byte1 << 4) + byte0);
    return temp;
}

// Transfer to 2-byte
static unsigned short word_transfer(char char3, char char2, char char1, char char0)
{
    unsigned short temp;
    unsigned char byte3, byte2, byte1, byte0;
    byte3 = ascii_to_hex(char3);
    byte2 = ascii_to_hex(char2);
    byte1 = ascii_to_hex(char1);
    byte0 = ascii_to_hex(char0);
    temp = combine_byte_into_word(byte3, byte2, byte1, byte0);
    return temp;
}

// Transfer to 1-byte
static unsigned char byte_transfer(char char1, char char0)
{
    unsigned char temp;
    unsigned char byte1, byte0;;
    byte1 = ascii_to_hex(char1);
    byte0 = ascii_to_hex(char0);
    byte1 &= 0xf;
    byte0 &= 0xf;
    temp = (unsigned char)((byte1 << 4) + byte0);
    return temp;
}

// Find position of seperator
static int find_separator(char *buf, char sep, int max_count)
{
    int i = 0;

    while (i < max_count) {
        if (*(buf + i) == sep)
            break;
        i++;
    }

    return i;
}

// Get a new hex line
static int get_new_line(char *newline)
{
    int s1, s2;
    char *ptr;

    if (next_data_pos > hex_data_size)
        return 0;

    ptr = hex_data_buf + next_data_pos;

    s1 = find_separator(ptr, ':', hex_data_size - next_data_pos);
    s2 = find_separator(ptr + s1 + 1, ':', hex_data_size - next_data_pos - s1);

    memset(newline, 0, 256);
    memcpy(newline, ptr + s1, (s2 - s1 + 1));

    next_data_pos += s2 + 1;

    return (s2 - s1);
}

// Parse a new hex line
static void parse_program_new_line(struct p6_data *pdata, char *line)
{
    int line_word_count;
    int line_type;

    line_type = byte_transfer(line[7], line[8]); // line type
    line_word_count = word_count_analysis(line[1], line[2]); // line data count in word (two-byte)

    if (line_type == 4) { // high addr
        data_high_addr = word_transfer(line[9], line[10], line[11], line[12]);
    }
    else if (line_type == 0) { // data
        unsigned int command, data, reply_command, reply_data;
        unsigned int flash_data;
        int i;

        data_low_addr = word_transfer(line[3], line[4], line[5], line[6]);

        for (i = 0; i < line_word_count; i++) {
            flash_data = word_transfer(line[9 + i * 4], line[10 + i * 4], line[11 + i * 4], line[12 + i * 4]);
            command = (0x1500 << 16) | (data_low_addr + i * 2);
            data = (data_high_addr << 16) | flash_data;
            reply_command = (0x9400 << 16) | (data_low_addr + i * 2);
            reply_data = data;
            // start programming
            send_data_to_power(pdata, command, data, reply_command, reply_data, upgrade_retries);
        }
    }
    else if (line_type == 5) { // do nothing

    }
    else if (line_type == 1) { // end line

    }
}

// Program hexdata to power
static void program_hexdata_to_power(struct p6_data *pdata)
{
    char new_line[256];

    next_data_pos = 0;

    while (get_new_line(new_line)) {
        parse_program_new_line(pdata, new_line);
    }
}

static u8 calc_upgrade_command_crc(u32 command, u32 data)
{
    u8 crc = 0;

    crc = (0x88 + 0x18 + 0xFF + 0x58 + 0xE6 + 0xC3 + 0x66) & 0xff;
    crc += ((command & 0xff) + (command >> 8 & 0xff) + (command >> 16 & 0xff) + (command >> 24 & 0xff)) & 0xff;
    crc += ((data & 0xff) + (data >> 8 & 0xff) + (data >> 16 & 0xff) + (data >> 24 & 0xff)) & 0xff;
    return crc;
}

static void pack_upgrade_command(u8 *buf, u32 command, u32 data)
{
    if (!buf)
        return;

    *buf++ = 0x88;
    *buf++ = (SEND_PACKET_ID >> 24) & 0xff;
    *buf++ = (SEND_PACKET_ID >> 16) & 0xff;
    *buf++ = (SEND_PACKET_ID >> 8) & 0xff;
    *buf++ = (SEND_PACKET_ID >> 0) & 0xff;
    *buf++ = 0xC3;
    *buf++ = (command >> 24) & 0xff;
    *buf++ = (command >> 16) & 0xff;
    *buf++ = (command >> 8) & 0xff;
    *buf++ = (command >> 0) & 0xff;
    *buf++ = (data >> 24) & 0xff;
    *buf++ = (data >> 16) & 0xff;
    *buf++ = (data >> 8) & 0xff;
    *buf++ = (data >> 0) & 0xff;
    *buf++ = calc_upgrade_command_crc(command, data);
    *buf = 0x66;
}

static int verify_reply_packet(u8 *buffer, u32 reply_command, u32 reply_data)
{
    int i = 0, start_pos = 0, verify_pass = 0;

    while (start_pos < REPLY_PACKET_BUFFER_SIZE - 12) {
        // Verify PACKET ID
        for (i = start_pos; i < REPLY_PACKET_BUFFER_SIZE - 12; i++) {
            if (*(u32 *)(buffer + i) == cpu_to_be32(REPLY_PACKET_ID)) {
                verify_pass = 1;
                break;
            }
        }
        start_pos = i + 4;

        // Verify reply_command
        if (verify_pass) {
            verify_pass = 0;
            for (i = start_pos; i < REPLY_PACKET_BUFFER_SIZE - 8; i++) {
                if (*(u32 *)(buffer + i) == cpu_to_be32(reply_command)) {
                    verify_pass = 1;
                    break;
                }
            }
            start_pos = i + 4;
        }

        // Verify reply_data
        if (verify_pass) {
            verify_pass = 0;
            for (i = start_pos; i < REPLY_PACKET_BUFFER_SIZE - 4; i++) {
                if (*(u32 *)(buffer + i) == cpu_to_be32(reply_data)) {
                    verify_pass = 1;
                    break;
                }
            }
            start_pos = i + 4;
        }

        if (verify_pass)
            break;
    }

    return verify_pass;
}

static int send_command_to_power(struct p6_data *pdata, u32 command, u32 data, u32 reply_command, u32 reply_data, int retries_count)
{
    struct i2c_client *client = pdata->client;
    u8 command_packet_buffer[COMMAND_PACKET_BUFFER_SIZE];
    u8 reply_packet_buffer[REPLY_PACKET_BUFFER_SIZE];
    int status;

    while (retries_count--) {
        // Send command & data packet
        pack_upgrade_command(command_packet_buffer, command, data);
        i2c_master_send(client,  command_packet_buffer, 16);

        // Waiting and verifying reply
        memset(reply_packet_buffer, 0, REPLY_PACKET_BUFFER_SIZE);
        i2c_master_recv(client, reply_packet_buffer, REPLY_PACKET_BUFFER_SIZE);

        if ((status = verify_reply_packet(reply_packet_buffer, reply_command, reply_data)))
            break;

        upgrade_cmd_retry_count++;
    }

    if ((retries_count == 0) && (status == 0))
        upgrade_fail_count++;
    else
        upgrade_success_count++;

    update_upgrade_status(command, data, status);

    return status;
}

static int send_data_to_power(struct p6_data *pdata, u32 command, u32 data, u32 reply_command, u32 reply_data, int retries_count)
{
    struct i2c_client *client = pdata->client;
    u8 command_packet_buffer[COMMAND_PACKET_BUFFER_SIZE];
    u8 reply_packet_buffer[REPLY_PACKET_BUFFER_SIZE];
    int status;

    // Send command & data packet just once
    pack_upgrade_command(command_packet_buffer, command, data);
    i2c_master_send(client,  command_packet_buffer, 16);

    // Waiting and verifying reply
    while (retries_count--) {
        memset(reply_packet_buffer, 0, REPLY_PACKET_BUFFER_SIZE);
        i2c_master_recv(client, reply_packet_buffer, REPLY_PACKET_BUFFER_SIZE);

        if ((status = verify_reply_packet(reply_packet_buffer, reply_command, reply_data)))
            break;

        upgrade_data_retry_count++;
    }

    if ((retries_count == 0) && (status == 0))
        upgrade_fail_count++;
    else
        upgrade_success_count++;

    update_upgrade_status(command, data, status);

    return status;
}

static void reset_power_to_execute(struct p6_data *pdata)
{
    send_command_to_power(pdata, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 1);
}

static ssize_t upgrade_power(struct p6_data *pdata, const char *buf, size_t count)
{
    int status;

    upgrade_started = 1;
    upgrade_finished = 0;

    // Transfer from boot state to programming state
    send_command_to_power(pdata, 0x30000000, 0x00005555, 0xB0000000, 0x00005555, upgrade_retries);
    send_command_to_power(pdata, 0x30000000, 0x0000AAAA, 0xB0000000, 0x0000AAAA, upgrade_retries);
    send_command_to_power(pdata, 0x31000000, 0x00000000, 0xB1000000, 0x00000000, upgrade_retries);
    send_command_to_power(pdata, 0x32000000, 0x00000000, 0xB2000000, 0x00000000, upgrade_retries);
    send_command_to_power(pdata, 0x33000000, 0x00007891, 0xB3000000, 0x000000FF, upgrade_retries);
    send_command_to_power(pdata, 0x34000000, 0x0000223A, 0x94000000, 0x0000223A, upgrade_retries);

    // Start programming
    program_hexdata_to_power(pdata);

    // Transfer to verifying state
    send_command_to_power(pdata, 0x16000000, 0x00000000, 0xB7000000, 0x00000000, upgrade_retries);

    // Enter success state
    status = send_command_to_power(pdata, 0x37000000, 0x00000001, 0xB7000000, 0x00000001, upgrade_retries);

    // Reset power to execute new firmware
    reset_power_to_execute(pdata);

    // Update status
    upgrade_started = 0;
    upgrade_finished = 1;
    update_upgrade_status(0x37000000, 0x00000001, status);

    return count;
}
