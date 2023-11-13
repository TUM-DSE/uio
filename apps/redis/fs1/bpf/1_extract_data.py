import os


def get_statistic(data_line: str):
    data = (data_line.split(":")[1]).split("requests per second")[0]
    return float(data.strip())


def parse_data(label: str, data_type: str, instruction: str):

    with open(f"{label}-{data_type}-{instruction}-report.txt", encoding="utf8") as data_file:
        lines = data_file.readlines()

        statistics = []
        for line in lines:
            if line.find("requests per second, p50=") >= 0:
                statistics.append(get_statistic(line))

        return statistics

def write_data(statistics, csv_name, program_names, instructions):
    with open(f"data-{csv_name}.csv", mode="w", encoding="utf8") as data_file:
        data_file.write("Instruction, Interpretation, JiT\n")
        
        for program_name in program_names:
            jits = statistics[program_name]["jit"]
            interpreters = statistics[program_name]["interpreter"]

            for instruction in instructions:
                for index in range(len(jits[instruction])):
                    data_file.write(f"Redis - {instruction} + {program_name.capitalize()}, {interpreters[instruction][index]}, {jits[instruction][index]}\n")

if __name__ == "__main__":
    result = {}

    instructions = ["SET", "GET", "INCR"]

    for (dirpath, dirnames, filenames) in os.walk("."):
        for file in filenames:
            if file.endswith(".txt"):
                filenames = file.split("-")
                
                filename = filenames[0]
                key = filename
                data_type = filenames[1]

                if key not in result:
                    result[key] = {}
                if data_type not in result[key]:
                    result[key][data_type] = {}

                for instruction in instructions:
                    result[key][data_type][instruction] = parse_data(filename, data_type, instruction)
    
    print(result)
    write_data(result, "redis", ["dummy_count", "count", "no_hook"], instructions)

