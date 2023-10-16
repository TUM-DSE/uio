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


        return sum(statistics) / len(statistics)

def write_data(statistics, type_name, keys, substract_base=True):
    with open(f"data-{type_name}.csv", mode="w", encoding="utf8") as data_file:
        data_file.write("Instruction Interpretation JiT\n")
        
        keys.reverse()
        for key in keys:
            inter = statistics[f'test_{key}']['interpreter'] - (statistics[f'test_for_loop']['interpreter'] if substract_base else 0)
            jit = statistics[f'test_{key}']['jit'] - (statistics[f'test_for_loop']['jit'] if substract_base else 0)
            data_file.write(f"{key.upper()} {round(inter, 1)} {round(jit, 1)}\n")
        data_file.write("DUMMY 0 0")

if __name__ == "__main__":
    result = {}

    for (dirpath, dirnames, filenames) in os.walk("."):
        for file in filenames:
            if file.endswith(".txt"):
                filenames = file.split("-")
                
                label = filenames[0]
                data_type = filenames[1]

                if label not in result:
                    result[label] = {}
                
                result[label][data_type] = parse_data(label, data_type)
    
    write_data(result, "base-case", ["for_loop"], False)
    write_data(result, "arithmetic", ["add", "sub", "mul", "div", "mod"])
    write_data(result, "others", ["or", "and", "xor", "left_shift", "right_shift", "neg", "assign"])

    print(result)
