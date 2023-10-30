import os


def get_statistic(data_line: str):
    parsed = data_line.split("Took: ")
    data = parsed[1].split("ns")

    return int(data[0].strip())


def parse_data(label: str, data_type: str):

    with open(f"{label}-{data_type}-report.txt", encoding="utf8") as data_file:
        lines = data_file.readlines()

        statistics = []
        for line in lines:
            if line.find("Took: ") > 0:
                statistics.append(get_statistic(line))

        return statistics

def write_data(statistics, csv_name, program_names):
    with open(f"data-{csv_name}.csv", mode="w", encoding="utf8") as data_file:
        data_file.write("Instruction Interpretation JiT\n")
        
        for program_name in program_names:
            jits = statistics[program_name]["jit"]
            interpreters = statistics[program_name]["interpreter"]

            for index in range(len(jits)):
                data_file.write(f"{program_name} {interpreters[index]} {jits[index]}\n")

if __name__ == "__main__":
    result = {}

    for (dirpath, dirnames, filenames) in os.walk("."):
        for file in filenames:
            if file.endswith(".txt"):
                filenames = file.split("-")
                
                filename = filenames[0]
                key = filename.split("test_")[1]
                data_type = filenames[1]

                if key not in result:
                    result[key] = {}
                
                result[key][data_type] = parse_data(filename, data_type)
    
    print(result)
    write_data(result, "microbenchmark", ["for_loop",
                                      "add", "sub", "mul", "div", "mod", "or", "and", "xor", "assign", "left_shift", "right_shift", "neg",
                                      "load_8", "load_16", "load_32", "load_64", "store_8", "store_16", "store_32", "store_64"])

