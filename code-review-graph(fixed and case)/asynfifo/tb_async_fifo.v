// ============================================================
// 异步 FIFO 测试平台
// ============================================================

`timescale 1ns / 1ps

module tb_async_fifo;

    // 参数
    parameter DATA_WIDTH = 8;
    parameter FIFO_DEPTH = 16;
    parameter CLK_WR_PERIOD = 10;  // 写时钟 100MHz
    parameter CLK_RD_PERIOD = 17;  // 读时钟 ~58.8MHz (异步)

    // 信号
    reg                  wr_clk, rd_clk;
    reg                  wr_rst_n, rd_rst_n;
    reg                  wr_en, rd_en;
    reg  [DATA_WIDTH-1:0] wr_data;
    wire [DATA_WIDTH-1:0] rd_data;
    wire                 full, empty;

    // 期望读数据 (用于自动比对)
    reg  [DATA_WIDTH-1:0] expected_data;
    reg                   check_rd;

    integer wr_cnt, rd_cnt, error_cnt;
    integer i;

    // 待测模块
    async_fifo #(
        .DATA_WIDTH(DATA_WIDTH),
        .FIFO_DEPTH(FIFO_DEPTH)
    ) dut (
        .wr_clk   (wr_clk),
        .wr_rst_n (wr_rst_n),
        .wr_en    (wr_en),
        .wr_data  (wr_data),
        .full     (full),
        .rd_clk   (rd_clk),
        .rd_rst_n (rd_rst_n),
        .rd_en    (rd_en),
        .rd_data  (rd_data),
        .empty    (empty)
    );

    // ============================================================
    // 时钟生成
    // ============================================================
    initial wr_clk = 0;
    always #(CLK_WR_PERIOD/2) wr_clk = ~wr_clk;

    initial rd_clk = 0;
    always #(CLK_RD_PERIOD/2) rd_clk = ~rd_clk;

    // ============================================================
    // 写任务
    // ============================================================
    task write_fifo(input [DATA_WIDTH-1:0] data);
        begin
            @(posedge wr_clk);
            #1;
            wr_en   = 1'b1;
            wr_data = data;
            @(posedge wr_clk);
            #1;
            wr_en   = 1'b0;
            wr_data = {DATA_WIDTH{1'bx}};
            wr_cnt  = wr_cnt + 1;
        end
    endtask

    // ============================================================
    // 读任务 (自动比对)
    // ============================================================
    task read_fifo;
        begin
            @(posedge rd_clk);
            #1;
            rd_en    = 1'b1;
            @(posedge rd_clk);
            #1;
            rd_en    = 1'b0;
            expected_data = rd_cnt;
            if (rd_data !== expected_data) begin
                $display("[ERROR] @%0t: rd_data=0x%0h, expected=0x%0h",
                         $time, rd_data, expected_data);
                error_cnt = error_cnt + 1;
            end
            rd_cnt = rd_cnt + 1;
        end
    endtask

    // ============================================================
    // 主测试序列
    // ============================================================
    initial begin
        // 初始化
        wr_clk      = 0;
        rd_clk      = 0;
        wr_rst_n    = 0;
        rd_rst_n    = 0;
        wr_en       = 0;
        rd_en       = 0;
        wr_data     = {DATA_WIDTH{1'b0}};
        wr_cnt      = 0;
        rd_cnt      = 0;
        error_cnt   = 0;
        check_rd    = 0;

        $display("");
        $display("========================================");
        $display(" 异步 FIFO 测试开始");
        $display("  DATA_WIDTH = %0d, FIFO_DEPTH = %0d", DATA_WIDTH, FIFO_DEPTH);
        $display("  WR_CLK = %0dns, RD_CLK = %0dns", CLK_WR_PERIOD, CLK_RD_PERIOD);
        $display("========================================");
        $display("");

        // 复位
        repeat (5) @(posedge wr_clk);
        repeat (5) @(posedge rd_clk);
        wr_rst_n = 1;
        rd_rst_n = 1;
        repeat (3) @(posedge wr_clk);

        // --------------------------------------------------
        // 测试 1: 复位后空标志
        // --------------------------------------------------
        $display("[TEST 1] 复位后 empty 标志检查");
        if (empty !== 1'b1) begin
            $display("[ERROR] 复位后 empty 应为 1");
            error_cnt = error_cnt + 1;
        end else begin
            $display("[OK]    empty = 1 (正确)");
        end
        if (full !== 1'b0) begin
            $display("[ERROR] 复位后 full 应为 0");
            error_cnt = error_cnt + 1;
        end else begin
            $display("[OK]    full = 0 (正确)");
        end
        $display("");

        // --------------------------------------------------
        // 测试 2: 写 FIFO_DEPTH/2 个数据并读回
        // --------------------------------------------------
        $display("[TEST 2] 写入 %0d 个数据并读回验证", FIFO_DEPTH/2);
        for (i = 0; i < FIFO_DEPTH/2; i = i + 1) begin
            write_fifo(i);
        end
        $display("  写入完成, full=%0b, empty=%0b", full, empty);

        // 间隔一些时钟，模拟异步
        repeat (3) @(posedge rd_clk);

        for (i = 0; i < FIFO_DEPTH/2; i = i + 1) begin
            read_fifo;
        end
        $display("  读取完成, empty=%0b", empty);
        if (empty !== 1'b1) begin
            $display("[ERROR] 读取所有数据后 empty 应为 1");
            error_cnt = error_cnt + 1;
        end else begin
            $display("[OK]    empty = 1 (正确)");
        end
        $display("");

        // --------------------------------------------------
        // 测试 3: 写满 FIFO
        // --------------------------------------------------
        $display("[TEST 3] 写满 FIFO (写入 %0d 个数据)", FIFO_DEPTH);
        for (i = 0; i < FIFO_DEPTH; i = i + 1) begin
            write_fifo(i + 100);
        end
        repeat (3) @(posedge wr_clk);
        #1;
        if (full !== 1'b1) begin
            $display("[ERROR] 写满后 full 应为 1");
            error_cnt = error_cnt + 1;
        end else begin
            $display("[OK]    full = 1 (正确)");
        end

        // 满时再写一个，不应写入
        $display("  满时尝试写入, full=%0b", full);
        write_fifo(8'hFF);
        $display("");

        // --------------------------------------------------
        // 测试 4: 全部读出
        // --------------------------------------------------
        $display("[TEST 4] 读取所有数据并验证");
        rd_cnt = 100;  // 写入时偏移了 100
        for (i = 0; i < FIFO_DEPTH; i = i + 1) begin
            read_fifo;
        end
        if (empty !== 1'b1) begin
            $display("[ERROR] 全部读取后 empty 应为 1");
            error_cnt = error_cnt + 1;
        end else begin
            $display("[OK]    empty = 1 (正确)");
        end
        $display("");

        // --------------------------------------------------
        // 测试 5: 连续写 + 同时读 (流水线)
        // --------------------------------------------------
        $display("[TEST 5] 同时写/读流水线测试");
        wr_cnt = 0;
        rd_cnt = 200;
        // 先写入 8 个
        for (i = 0; i < 8; i = i + 1) begin
            write_fifo(i + 200);
        end

        // 同时读写 (读写时钟域异步)
        fork
            begin
                for (i = 0; i < 16; i = i + 1) begin
                    write_fifo(i + 208);
                    repeat ($urandom % 3) @(posedge wr_clk);
                end
            end
            begin
                repeat (3) @(posedge rd_clk);
                rd_cnt = 200;
                for (i = 0; i < 16; i = i + 1) begin
                    read_fifo;
                    repeat ($urandom % 4) @(posedge rd_clk);
                end
            end
        join

        repeat (10) @(posedge rd_clk);
        #1;
        $display("");

        // ============================================================
        // 测试结果汇总
        // ============================================================
        $display("========================================");
        if (error_cnt == 0) begin
            $display(" 测试通过! 所有检查均正确.");
        end else begin
            $display(" 测试失败! 发现 %0d 个错误.", error_cnt);
        end
        $display("========================================");
        $display("");

        #1000;
        $finish;
    end

    // ============================================================
    // VCD 波形输出
    // ============================================================
    initial begin
        $dumpfile("tb_async_fifo.vcd");
        $dumpvars(0, tb_async_fifo);
    end

endmodule
