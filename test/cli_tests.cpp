#include <gtest/gtest.h>

#include "cli.hpp"

TEST(CliParserTest, ParsesBuy)
{
    const Command cmd = parse_command("buy 100 25");

    EXPECT_EQ(cmd.type, CommandType::ADD);
    EXPECT_EQ(cmd.order.side, Side::BUY);
    EXPECT_EQ(cmd.order.price, 100U);
    EXPECT_EQ(cmd.order.quantity, 25U);
}

TEST(CliParserTest, ParsesSell)
{
    const Command cmd = parse_command("sell 101 30");

    EXPECT_EQ(cmd.type, CommandType::ADD);
    EXPECT_EQ(cmd.order.side, Side::SELL);
    EXPECT_EQ(cmd.order.price, 101U);
    EXPECT_EQ(cmd.order.quantity, 30U);
}

TEST(CliParserTest, ParsesCancel)
{
    const Command cmd = parse_command("cancel 42");

    EXPECT_EQ(cmd.type, CommandType::CANCEL);
    EXPECT_EQ(cmd.order.order_id, 42U);
}

TEST(CliParserTest, ParsesBook)
{
    const Command cmd = parse_command("book");

    EXPECT_EQ(cmd.type, CommandType::BOOK);
}

TEST(CliParserTest, ParsesHelp)
{
    const Command cmd = parse_command("help");

    EXPECT_EQ(cmd.type, CommandType::HELP);
}

TEST(CliParserTest, RejectsUnknownCommand)
{
    const Command cmd = parse_command("foobar");

    EXPECT_EQ(cmd.type, CommandType::UNKNOWN);
}

TEST(CliParserTest, RejectsMissingBuyArguments)
{
    const Command cmd = parse_command("buy 100");

    EXPECT_EQ(cmd.type, CommandType::UNKNOWN);
}

TEST(CliParserTest, RejectsMissingSellArguments)
{
    const Command cmd = parse_command("sell 100");

    EXPECT_EQ(cmd.type, CommandType::UNKNOWN);
}

TEST(CliParserTest, RejectsMissingCancelArgument)
{
    const Command cmd = parse_command("cancel");

    EXPECT_EQ(cmd.type, CommandType::UNKNOWN);
}

TEST(CliParserTest, RejectsExtraArguments)
{
    EXPECT_EQ(parse_command("buy 100 10 extra").type, CommandType::UNKNOWN);
    EXPECT_EQ(parse_command("sell 100 10 extra").type, CommandType::UNKNOWN);
    EXPECT_EQ(parse_command("cancel 1 extra").type, CommandType::UNKNOWN);
    EXPECT_EQ(parse_command("book extra").type, CommandType::UNKNOWN);
    EXPECT_EQ(parse_command("help extra").type, CommandType::UNKNOWN);
}

TEST(CliParserTest, RejectsZeroPrice)
{
    const Command cmd = parse_command("buy 0 10");

    EXPECT_EQ(cmd.type, CommandType::UNKNOWN);
}

TEST(CliParserTest, RejectsZeroQuantity)
{
    const Command cmd = parse_command("sell 100 0");

    EXPECT_EQ(cmd.type, CommandType::UNKNOWN);
}

TEST(CliParserTest, RejectsPriceOverflow)
{
    const Command cmd = parse_command("buy 4294967296 10");

    EXPECT_EQ(cmd.type, CommandType::UNKNOWN);
}

TEST(CliParserTest, RejectsQuantityOverflow)
{
    const Command cmd = parse_command("sell 100 4294967296");

    EXPECT_EQ(cmd.type, CommandType::UNKNOWN);
}

TEST(CliParserTest, AcceptsLeadingAndTrailingWhitespace)
{
    const Command cmd = parse_command("   buy 100 10   ");

    EXPECT_EQ(cmd.type, CommandType::ADD);
    EXPECT_EQ(cmd.order.side, Side::BUY);
    EXPECT_EQ(cmd.order.price, 100U);
    EXPECT_EQ(cmd.order.quantity, 10U);
}

TEST(CliParserTest, RejectsNegativeBuyPrice)
{
    const Command cmd = parse_command("buy -100 10");

    EXPECT_EQ(cmd.type, CommandType::UNKNOWN);
}

TEST(CliParserTest, RejectsNegativeBuyQuantity)
{
    const Command cmd = parse_command("buy 100 -10");

    EXPECT_EQ(cmd.type, CommandType::UNKNOWN);
}

TEST(CliParserTest, RejectsNegativeSellPrice)
{
    const Command cmd = parse_command("sell -100 10");

    EXPECT_EQ(cmd.type, CommandType::UNKNOWN);
}

TEST(CliParserTest, RejectsNegativeSellQuantity)
{
    const Command cmd = parse_command("sell 100 -10");

    EXPECT_EQ(cmd.type, CommandType::UNKNOWN);
}

TEST(CliParserTest, RejectsNegativeCancelId)
{
    const Command cmd = parse_command("cancel -1");

    EXPECT_EQ(cmd.type, CommandType::UNKNOWN);
}