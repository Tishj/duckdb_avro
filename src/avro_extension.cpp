#define DUCKDB_EXTENSION_MAIN

#include "avro_extension.hpp"

#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/common/multi_file/multi_file_reader.hpp"
#include "duckdb/function/scalar_function.hpp"
#include "duckdb/main/extension_util.hpp"
#include "duckdb/parser/parsed_data/create_scalar_function_info.hpp"
#include "include/avro_reader.hpp"

#include "utf8proc_wrapper.hpp"

#include <avro.h>

namespace duckdb {

struct AvroBindData : public FunctionData {
public:
	void Initialize(shared_ptr<AvroReader> reader) {
		initial_reader = std::move(reader);
	}

	void Initialize(ClientContext &, unique_ptr<AvroUnionData> &union_data) {
		throw InternalException("union_by_name not supported");
	}

	void Initialize(ClientContext &, shared_ptr<AvroReader> reader) {
		Initialize(reader);
	}

	bool Equals(const FunctionData &other_p) const override {
		throw NotImplementedException("AvroBindData::Equals");
	}

	unique_ptr<FunctionData> Copy() const override {
		throw NotImplementedException("AvroBindData::Copy");
	}
public:
	//shared_ptr<MultiFileList> file_list;
	//unique_ptr<MultiFileReader> multi_file_reader;
	//MultiFileReaderBindData reader_bind;
	vector<string> names;
	vector<LogicalType> types;
	shared_ptr<AvroReader> initial_reader;

	// unused
	vector<unique_ptr<AvroUnionData>> union_readers;
};

static unique_ptr<FunctionData> AvroBindFunction(ClientContext &context, TableFunctionBindInput &input,
                                                 vector<LogicalType> &return_types, vector<string> &names) {
	auto &filename = input.inputs[0];
	auto result = make_uniq<AvroBindData>();
	//result->multi_file_reader = MultiFileReader::Create(input.table_function);

	for (auto &kv : input.named_parameters) {
		if (kv.second.IsNull()) {
			throw BinderException("Cannot use NULL as function argument");
		}
		auto loption = StringUtil::Lower(kv.first);
		//if (result->multi_file_reader->ParseOption(kv.first, kv.second, result->avro_options.file_options, context)) {
		//	continue;
		//}
		continue;
		throw InternalException("Unrecognized option %s", loption.c_str());
	}

	//if (result->avro_options.file_options.union_by_name) {
	//	throw NotImplementedException("union_by_name for Avro reads");
	//}
	//result->file_list = result->multi_file_reader->CreateFileList(context, filename);

	//result->reader_bind = result->multi_file_reader->BindReader<AvroReader>(
	//    context, result->types, result->names, *result->file_list, *result, result->avro_options);

	return_types = result->types;
	names = result->names;

	return result;
}

struct AvroGlobalState : public GlobalTableFunctionState {
	mutex lock;

	//MultiFileListScanData scan_data;
	shared_ptr<AvroReader> reader;

	vector<ColumnIndex> column_indexes;
	optional_ptr<TableFilterSet> filters;
};

//static bool AvroNextFile(ClientContext &context, const AvroBindData &bind_data, AvroGlobalState &global_state,
//                         shared_ptr<AvroReader> initial_reader) {
//	unique_lock<mutex> parallel_lock(global_state.lock);

//	string file;
//	if (!bind_data.file_list->Scan(global_state.scan_data, file)) {
//		return false;
//	}

//	// re-use initial reader for first file, no need to parse metadata again
//	if (initial_reader) {
//		D_ASSERT(file == initial_reader->filename);
//		global_state.reader = initial_reader;
//	} else {
//		auto new_reader = make_shared_ptr<AvroReader>(context, file, bind_data.avro_options);
//		if (new_reader->duckdb_type != global_state.reader->duckdb_type) {
//			throw InvalidInputException("Schema of file %s (%s) differs from first file %s (%s)", new_reader->filename,
//			                            new_reader->duckdb_type.ToString(), global_state.reader->filename,
//			                            global_state.reader->duckdb_type.ToString());
//		}
//		global_state.reader = std::move(new_reader);
//	}

//	auto columns = MultiFileColumnDefinition::ColumnsFromNamesAndTypes(bind_data.names, bind_data.types);
//	bind_data.multi_file_reader->InitializeReader(*global_state.reader, bind_data.avro_options.file_options,
//	                                              bind_data.reader_bind, columns, global_state.column_indexes,
//	                                              global_state.filters, file, context, nullptr);
//	return true;
//}

static void AvroTableFunction(ClientContext &context, TableFunctionInput &data, DataChunk &output) {
	auto &bind_data = data.bind_data->Cast<AvroBindData>();
	auto &global_state = data.global_state->Cast<AvroGlobalState>();
	//do {
	//	output.Reset();
	//	global_state.reader->Read(output, global_state.column_indexes);
	//	bind_data.multi_file_reader->FinalizeChunk(context, bind_data.reader_bind, global_state.reader->reader_data,
	//	                                           output, nullptr);
	//	if (output.size() > 0) {
	//		return;
	//	}
	//	if (!AvroNextFile(context, bind_data, global_state, nullptr)) {
	//		return;
	//	}
	//} while (true);
}

unique_ptr<GlobalTableFunctionState> AvroGlobalInit(ClientContext &context, TableFunctionInitInput &input) {
	auto global_state_result = make_uniq<AvroGlobalState>();
	auto &global_state = *global_state_result;
	auto &bind_data = input.bind_data->Cast<AvroBindData>();

	global_state.column_indexes = input.column_indexes;
	global_state.filters = input.filters;

	//bind_data.file_list->InitializeScan(global_state.scan_data);
	//if (!AvroNextFile(context, bind_data, global_state, bind_data.initial_reader)) {
	//	throw InternalException("Cannot scan files");
	//}
	return global_state_result;
}

static void LoadInternal(DatabaseInstance &instance) {
	// Register a scalar function
	auto table_function =
	    TableFunction("read_avro", {LogicalType::VARCHAR}, AvroTableFunction, AvroBindFunction, AvroGlobalInit);
	table_function.projection_pushdown = true;
	MultiFileReader::AddParameters(table_function);
	ExtensionUtil::RegisterFunction(instance, MultiFileReader::CreateFunctionSet(table_function));
}

void AvroExtension::Load(DuckDB &db) {
	LoadInternal(*db.instance);
}
std::string AvroExtension::Name() {
	return "avro";
}

std::string AvroExtension::Version() const {
#ifdef EXT_VERSION_AVRO
	return EXT_VERSION_AVRO;
#else
	return "";
#endif
}

} // namespace duckdb

extern "C" {

DUCKDB_EXTENSION_API void avro_init(duckdb::DatabaseInstance &db) {
	duckdb::DuckDB db_wrapper(db);
	db_wrapper.LoadExtension<duckdb::AvroExtension>();
}

DUCKDB_EXTENSION_API const char *avro_version() {
	return duckdb::DuckDB::LibraryVersion();
}
}

#ifndef DUCKDB_EXTENSION_MAIN
#error DUCKDB_EXTENSION_MAIN not defined
#endif
