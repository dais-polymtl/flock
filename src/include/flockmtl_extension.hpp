#pragma once

#include "flockmtl/core/common.hpp"

namespace duckdb {

class FlockmtlExtension : public Extension {
public:
    void Load(ExtensionLoader& db) override;
    std::string Name() override;
    std::string Version() const override;
};

BoundStatement duck_bind(ClientContext& context, Binder& binder, OperatorExtensionInfo* info, SQLStatement& statement);

struct DuckOperatorExtension : public OperatorExtension {
    DuckOperatorExtension() : OperatorExtension() { Bind = duck_bind; }

    std::string GetName() override { return "duck"; }

    unique_ptr<LogicalExtensionOperator> Deserialize(Deserializer& deserializer) override {
        throw InternalException("duck operator should not be serialized");
    }
};

ParserExtensionParseResult duck_parse(ParserExtensionInfo*, const std::string& query);

ParserExtensionPlanResult duck_plan(ParserExtensionInfo*, ClientContext&, unique_ptr<ParserExtensionParseData>);

struct DuckParserExtension : public ParserExtension {
    DuckParserExtension() : ParserExtension() {
        parse_function = duck_parse;
        plan_function = duck_plan;
    }
};

struct DuckParseData : ParserExtensionParseData {
    unique_ptr<SQLStatement> statement;

    unique_ptr<ParserExtensionParseData> Copy() const override {
        return make_uniq_base<ParserExtensionParseData, DuckParseData>(statement->Copy());
    }

    virtual string ToString() const override { return "DuckParseData"; }

    DuckParseData(unique_ptr<SQLStatement> statement) : statement(std::move(statement)) {}
};

class DuckState : public ClientContextState {
public:
    explicit DuckState(unique_ptr<ParserExtensionParseData> parse_data) : parse_data(std::move(parse_data)) {}

    void QueryEnd() override { parse_data.reset(); }

    unique_ptr<ParserExtensionParseData> parse_data;
};

}// namespace duckdb
