#include "financial/common/generator.hpp"
#include "financial/oracle/pipeline.hpp"

#include <iostream>

int main()
{
    const auto reference = financial::common::ReferenceData::make_default();
    const auto workload = financial::common::generate_workload({.records = 1000});
    const auto output = financial::oracle::process_records(workload.records, reference);
    std::cout << "solution=sequential_oracle\n";
    std::cout << "records=" << output.records.size() << '\n';
    std::cout << "input_bytes=" << output.input_bytes << '\n';
    std::cout << "output_bytes=" << output.output_bytes << '\n';
    std::cout << "checksum=" << output.checksum << '\n';
    return output.records.size() == workload.records.size() ? 0 : 1;
}
