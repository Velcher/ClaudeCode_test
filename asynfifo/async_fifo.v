// ============================================================
// 异步 FIFO (Asynchronous FIFO)
// 特性:
//   - 参数化数据宽度 (DATA_WIDTH) 和深度 (FIFO_DEPTH)
//   - 格雷码指针实现安全跨时钟域传输
//   - 双级同步器消除亚稳态
//   - 空/满标志生成
//   - 可配置写/读时钟域
// ============================================================

module async_fifo #(
    parameter DATA_WIDTH = 8,
    parameter FIFO_DEPTH = 16
) (
    // 写时钟域
    input  wire                wr_clk,
    input  wire                wr_rst_n,
    input  wire                wr_en,
    input  wire [DATA_WIDTH-1:0] wr_data,
    output wire                full,

    // 读时钟域
    input  wire                rd_clk,
    input  wire                rd_rst_n,
    input  wire                rd_en,
    output wire [DATA_WIDTH-1:0] rd_data,
    output wire                empty
);

    // 地址宽度
    localparam ADDR_WIDTH = $clog2(FIFO_DEPTH);
    // 用于满/空判断需要多1位
    localparam PTR_WIDTH  = ADDR_WIDTH + 1;

    // ============================================================
    // 1. 双端口 RAM
    // ============================================================
    reg [DATA_WIDTH-1:0] mem [0:FIFO_DEPTH-1];
    wire [ADDR_WIDTH-1:0] wr_addr;
    reg  [ADDR_WIDTH-1:0] rd_addr;

    // 写操作
    always @(posedge wr_clk) begin
        if (wr_en && !full)
            mem[wr_addr] <= wr_data;
    end

    reg [DATA_WIDTH-1:0] rd_data_reg;

    // 读操作：先读出当前地址数据，再递增地址
    always @(posedge rd_clk or negedge rd_rst_n) begin
        if (!rd_rst_n) begin
            rd_data_reg <= {DATA_WIDTH{1'b0}};
            rd_addr <= {ADDR_WIDTH{1'b0}};
        end else if (rd_en && !empty) begin
            rd_data_reg <= mem[rd_addr];
            rd_addr <= rd_addr + 1'b1;
        end
    end

    assign rd_data = rd_data_reg;

    // ============================================================
    // 2. 指针管理 (二进制 + 格雷码)
    // ============================================================
    reg  [PTR_WIDTH-1:0] wr_ptr;
    reg  [PTR_WIDTH-1:0] rd_ptr;
    wire [PTR_WIDTH-1:0] wr_ptr_next, rd_ptr_next;
    wire [PTR_WIDTH-1:0] wr_gray, rd_gray;

    // 写指针
    assign wr_ptr_next = wr_ptr + {{PTR_WIDTH-1{1'b0}}, (wr_en && !full)};
    always @(posedge wr_clk or negedge wr_rst_n) begin
        if (!wr_rst_n)
            wr_ptr <= {PTR_WIDTH{1'b0}};
        else
            wr_ptr <= wr_ptr_next;
    end

    // 读指针
    assign rd_ptr_next = rd_ptr + {{PTR_WIDTH-1{1'b0}}, (rd_en && !empty)};
    always @(posedge rd_clk or negedge rd_rst_n) begin
        if (!rd_rst_n)
            rd_ptr <= {PTR_WIDTH{1'b0}};
        else
            rd_ptr <= rd_ptr_next;
    end

    // 二进制 → 格雷码
    assign wr_gray = wr_ptr ^ (wr_ptr >> 1);
    assign rd_gray = rd_ptr ^ (rd_ptr >> 1);

    // 写地址 (低位)
    assign wr_addr = wr_ptr[ADDR_WIDTH-1:0];

    // ============================================================
    // 3. 双级同步器 (跨时钟域)
    // ============================================================
    reg [PTR_WIDTH-1:0] wr_gray_sync1, wr_gray_sync2;
    reg [PTR_WIDTH-1:0] rd_gray_sync1, rd_gray_sync2;

    // 同步写指针 → 读时钟域
    always @(posedge rd_clk or negedge rd_rst_n) begin
        if (!rd_rst_n) begin
            wr_gray_sync1 <= {PTR_WIDTH{1'b0}};
            wr_gray_sync2 <= {PTR_WIDTH{1'b0}};
        end else begin
            wr_gray_sync1 <= wr_gray;
            wr_gray_sync2 <= wr_gray_sync1;
        end
    end

    // 同步读指针 → 写时钟域
    always @(posedge wr_clk or negedge wr_rst_n) begin
        if (!wr_rst_n) begin
            rd_gray_sync1 <= {PTR_WIDTH{1'b0}};
            rd_gray_sync2 <= {PTR_WIDTH{1'b0}};
        end else begin
            rd_gray_sync1 <= rd_gray;
            rd_gray_sync2 <= rd_gray_sync1;
        end
    end

    // ============================================================
    // 4. 跨时钟域同步后的指针 (格雷码)
    // ============================================================
    wire [PTR_WIDTH-1:0] wr_gray_sync  = wr_gray_sync2;
    wire [PTR_WIDTH-1:0] rd_gray_sync  = rd_gray_sync2;

    // ============================================================
    // 5. 空/满标志生成 (均在格雷码域比较)
    // ============================================================
    // 空: 读指针 (格雷码) 与同步过来的写指针 (格雷码) 相等
    assign empty = (rd_gray == wr_gray_sync);

    // 满: 写指针比读指针领先一圈
    //     格雷码下: 最高位相反，次高位相反，其余位相等
    assign full  = (wr_gray[PTR_WIDTH-1]   != rd_gray_sync[PTR_WIDTH-1]) &&
                   (wr_gray[PTR_WIDTH-2]   != rd_gray_sync[PTR_WIDTH-2]) &&
                   (wr_gray[PTR_WIDTH-3:0] == rd_gray_sync[PTR_WIDTH-3:0]);

endmodule
