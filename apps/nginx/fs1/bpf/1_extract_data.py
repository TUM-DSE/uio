import os


def get_statistic(data_line: str):
    data = data_line.split("Requests/sec: ")

    return float(data[1].strip())


def parse_data(label: str, data_type: str):

    with open(f"{label}-{data_type}-report.txt", encoding="utf8") as data_file:
        lines = data_file.readlines()

        statistics = []
        for line in lines:
            if line.find("Requests/sec:") >= 0:
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
                key = filename
                data_type = filenames[1]

                if key not in result:
                    result[key] = {}
                
                print(filename)
                result[key][data_type] = parse_data(filename, data_type)
    
    print(result)
    write_data(result, "nginx", ["nop", "count", "dummy_count", "no_hook"])

