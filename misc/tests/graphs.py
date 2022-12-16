#!/usr/bin/env python3

import pandas as pd
import re
import sys
from pathlib import Path
from typing import Any, List
from natsort import natsort_keygen
import warnings

from plot import (
    apply_aliases,
    catplot,
    column_alias,
    explode,
    sns,
    PAPER_MODE,
    plt,
    format,
    magnitude_formatter,
    change_width,
    apply_hatch
)
from plot import ROW_ALIASES, COLUMN_ALIASES, FORMATTER

if PAPER_MODE:
    out_format = ".pdf"
else:
    out_format = ".png"

palette = sns.color_palette("colorblind")
palette = [palette[-1], palette[1], palette[2]]

hatches = ["//", "..", ""]

ROW_ALIASES.update(
    {
        "direction": dict(read_mean="read", write_mean="write"),
        "system": {
            "ushell-console": "ushell",
            "ushell-console-nginx": "ushell + nginx load",
            "qemu_ssh_console": "linux + ssh",
            "ushell-init": "wo/ isolation",
            "redis_ushell_initrd_nohuman": "ushell nohuman",
            "redis_noshell_initrd_nohuman": "baseline",
            "sqlite_ushell_initrd_nohuman": "ushell nohuman",
            "sqlite_noshell_initrd_nohuman": "baseline",
            "nginx_ushell_initrd_nohuman": "ushell nohuman",
            "nginx_noshell_initrd_nohuman": "baseline",
            "nginx_ushell_initrd_lshuman": "ushell lshuman",
            "ushell_run": "run hello",
            "ushell-run-cached": "run hello (cached)",
            "uk-nginx-noshell-initrd": "nginx",
            "uk-nginx-ushell-initrd": "nginx ushell",
            "uk-redis-noshell-initrd": "redis",
            "uk-redis-ushell-initrd": "redis ushell",
            "uk-sqlite_benchmark-noshell-initrd": "sqlite",
            "uk-sqlite_benchmark-ushell-initrd": "sqlite ushell",
        },
        "ltoshell": {
            "noshell": "baseline",
            "noshell-lto": "baseline-lto",
        },
        "shell": {
            "noshell": "baseline",
        },
        "iotype": dict(
            direct="Direct/Block IO",
            file="File IO",
        ),
    }
)


COLUMN_ALIASES.update(
    {
        "ushell-console-seconds": "latency [ms]",
        "ushell-init-seconds": "time [ms]",
        "ushell_run-seconds": "time [ms]",
        "redis-requests": "requests/s",
        "nginx-requests": "requests/s",
        "sqlite-seconds": "time [s]",
        "image-size": "size [kB]",
    }
)
FORMATTER.update(
    {
        "iops": magnitude_formatter(3),
        "io_throughput": magnitude_formatter(6),
        "seconds": magnitude_formatter(-3),
        "kB": magnitude_formatter(3),
    }
)


def image_sizes(df: pd.DataFrame) -> Any:
    print("Size reduction")
    reduction = 100 * (1 - (df.new_size / df.old_size))
    print(reduction.describe())

    df2 = df.assign(reduction=reduction)
    not_effective = df2[df2.reduction <= 10]
    print(
        f"Size reduction smaller than 10%\n{not_effective}\n{not_effective.describe()}"
    )
    df_before = df.assign(when="before", container_size=lambda x: x.old_size / 10e6)
    df_after = df.assign(when="after", container_size=lambda x: x.new_size / 10e6)
    merged = pd.concat([df_before, df_after])

    sns.set(font_scale=1.3)
    sns.set_style("whitegrid")
    g = sns.boxplot(
        y=column_alias("container_size"),
        x=column_alias("when"),
        data=apply_aliases(merged),
        palette=None,
    )
    g.axes.set_xlabel("")
    g.set(ylim=(-1, 100))
    g.get_figure().set_figheight(3.3)
    plt.gcf().tight_layout()
    FONT_SIZE = 12
    g.annotate(
        "Lower is better",
        xycoords="axes fraction",
        xy=(0, 0),
        xytext=(1.02, 0.17),
        fontsize=FONT_SIZE,
        color="navy",
        weight="bold",
        rotation="vertical",
    )
    g.annotate(
        "",
        xycoords="axes fraction",
        xy=(1.04, 0.05),
        xytext=(1.04, 0.15),
        fontsize=FONT_SIZE,
        arrowprops=dict(arrowstyle="-|>", color="navy"),
    )
    # apply_to_graphs(g, legend=False, width=0.28)
    return plt.gcf()


def gobshit_to_stddev(df: pd.DataFrame) -> pd.DataFrame:
    df.insert(df.shape[1], "stddev", [0 for _ in range(df.shape[0])])

    def f(row: Any) -> Any:
        if row.direction == "read_mean":
            row.stddev = row.read_stddev
        elif row.direction == "write_mean":
            row.stddev = row.write_stddev
        return row

    df = df.apply(f, axis=1)
    del df["read_stddev"]
    del df["write_stddev"]
    return df


def stddev_to_series(df: pd.DataFrame, mean: str, stddev: str) -> pd.DataFrame:
    ret = pd.DataFrame()
    for _index, row in df.iterrows():
        samples = explode(row[mean], row[stddev])
        for sample in samples:
            row[mean] = sample
            ret = ret.append(row)
    del ret[stddev]
    return ret


def system_to_iotype(df: pd.DataFrame, value: str) -> pd.DataFrame:
    def f(row: Any) -> Any:
        if "direct" in row.system:
            return "direct"
        else:
            return "file"

    iotype = df.apply(f, axis=1)
    return df.assign(iotype=iotype)


def parse_app_system(df: pd.DataFrame) -> pd.DataFrame:
    def h(row: Any) -> Any:
        if "redis" in row.system:
            return "redis"
        elif "nginx" in row.system:
            return "nginx"
        elif "sqlite" in row.system:
            return "sqlite"
        elif "count" in row.system:
            return "count"

    app = df.apply(h, axis=1)
    ret = df.assign(app=app)

    def i(row: Any) -> Any:
        if "-set" in row.system:
            return "set"
        elif "-get" in row.system:
            return "get"
        else:
            return ""

    direction = df.apply(i, axis=1)
    ret = ret.assign(direction=direction)

    def j(row: Any) -> Any:
        if "-set" in row.system:
            return row.system[:-4]
        elif "-get" in row.system:
            return row.system[:-4]
        else:
            return row.system

    system = df.apply(j, axis=1)
    ret = ret.assign(system=system)

    def k(row: Any) -> Any:
        if "-lto" in row.system:
            return f"{row.app}-lto"
        else:
            return f"{row.app}"

    ltoapp = ret.apply(k, axis=1)
    ret = ret.assign(ltoapp=ltoapp)

    def f(row: Any) -> Any:
        if "noshell" in row.system:
            return "noshell"
        elif "ushell" in row.system:
            return "ushell"

    shell = df.apply(f, axis=1)
    ret = ret.assign(shell=shell)

    def l(row: Any) -> Any:
        if "-lto" in row.system:
            return f"{row.shell}-lto"
        else:
            return f"{row.shell}"

    ltoshell = ret.apply(l, axis=1)
    ret = ret.assign(ltoshell=ltoshell)

    def g(row: Any) -> Any:
        if "9p" in row.system:
            return "9p"
        elif "initrd" in row.system:
            return "initrd"

    rootfs = df.apply(g, axis=1)
    return ret.assign(rootfs=rootfs)


def fio(df: pd.DataFrame, what: str, value_name: str) -> Any:
    df = df[df["benchmark"] == what][df["system"] != "direct_host2"]
    df = df.melt(
        id_vars=["system", "benchmark", "Unnamed: 0", "write_stddev", "read_stddev"],
        var_name="direction",
        value_name=value_name,
    )
    df = gobshit_to_stddev(df)
    df = system_to_iotype(df, value_name)
    df = stddev_to_series(df, value_name, "stddev")

    directs = sum([int(t == "direct") for t in df["iotype"]])
    files = sum([int(t == "file") for t in df["iotype"]])
    g = catplot(
        data=apply_aliases(df),
        y=column_alias("system"),
        # order=systems_order(df),
        x=column_alias(value_name),
        hue=column_alias("direction"),
        kind="bar",
        ci="sd",  # show standard deviation! otherwise with_stddev_to_long_form does not work.
        height=2.3,
        aspect=2,
        palette=None,
        legend=False,
        row="iotype",
        sharex=True,
        sharey=False,
        facet_kws=dict({"gridspec_kws": {"height_ratios": [directs, files]}}),
    )
    # apply_to_graphs(g.ax, False, 0.285)
    g.axes[1][0].legend(
        loc="lower right", frameon=True, title=column_alias("direction")
    )
    # g.axes[0][0].set_title("Direct/Block IO", size=10)
    # g.axes[1][0].set_title("File IO", size=10)
    g.axes[0][0].set_ylabel("")
    g.axes[1][0].set_ylabel("")
    g.axes[0][0].grid()
    g.axes[1][0].grid()
    # g.axes[0][0].set_xlim([0, 200000])
    # g.axes[1][0].set_xlim([0, 30000])
    # if "iops" in value_name:
    # g.axes[0][0].set_xscale("log")
    # g.axes[1][0].set_xscale("log")
    FONT_SIZE = 7.5
    g.axes[1][0].annotate(
        "Higher is better",
        xycoords="axes fraction",
        xy=(0, 0),
        xytext=(-0.47, -0.25),
        fontsize=FONT_SIZE,
        color="navy",
        weight="bold",
    )
    g.axes[1][0].annotate(
        "",
        xycoords="axes fraction",
        xy=(-0.04, -0.23),
        xytext=(-0.13, -0.23),
        fontsize=FONT_SIZE,
        arrowprops=dict(arrowstyle="-|>", color="navy"),
    )
    format(g.axes[0][0].xaxis, value_name)
    format(g.axes[1][0].xaxis, value_name)
    return g


def sort(df: pd.DataFrame, systems: List[str]) -> pd.DataFrame:
    sparse = pd.concat([ df[df["system"] == n] for n in systems ])
    return pd.concat([ sparse, df ]).drop_duplicates(keep='first')


def console(df: pd.DataFrame, name: str, names: List[str] = []) -> Any:
    if len(names) == 0: names = [name]
    df = df.melt(id_vars=["Unnamed: 0"], var_name="system", value_name=f"{name}-seconds")
    df = pd.concat([ df[df["system"] == n] for n in names ])
    # df = df.append(dict(system=r"human", seconds=0.013), ignore_index=True)
    width = 3.3
    aspect = 2.0
    g = catplot(
        data=apply_aliases(df),
        y=column_alias("system"),
        # order=systems_order(df),
        x=column_alias(f"{name}-seconds"),
        kind="bar",
        ci="sd",  # show standard deviation! otherwise with_stddev_to_long_form does not work.
        height=width/aspect,
        aspect=aspect,
        palette=palette,
    )
    import plot
    plot.set_barplot_height(g.ax, 0.3)
    # apply_to_graphs(g.ax, False, 0.285)
    # g.ax.set_xscale("log")
    g.ax.set_ylabel("")

    FONT_SIZE = 9
    g.ax.annotate(
        "Lower is better",
        xycoords="axes points",
        xy=(0, 0),
        xytext=(1, -30),
        fontsize=FONT_SIZE,
        color="navy",
        weight="bold",
    )
    g.ax.annotate(
        "",
        xycoords="axes points",
        xy=(-15, -27),
        xytext=(0, -27),
        fontsize=FONT_SIZE,
        arrowprops=dict(arrowstyle="-|>", color="navy"),
    )

    g.despine()
    format(g.ax.xaxis, "seconds")
    return g


def images(df: pd.DataFrame, name: str, names: List[str] = []) -> Any:
    if len(names) == 0: names = [name]
    df = df.melt(id_vars=["Unnamed: 0"], var_name="system", value_name=f"image-size")
    df = parse_app_system(df)
    df = df.sort_values(by="shell")

    # df = pd.concat([ df[df["system"] == n] for n in names ])
    # df = df.append(dict(system=r"human", seconds=0.013), ignore_index=True)
    width = 2.8
    aspect = 1.5
    g = catplot(
        data=apply_aliases(df),
        y=column_alias("app"),
        # order=systems_order(df),
        x=column_alias(f"image-size"),
        kind="bar",
        hue="ltoshell",
        ci="sd",  # show standard deviation! otherwise with_stddev_to_long_form does not work.
        height=width/aspect,
        aspect=aspect,
        palette=palette,
    )
    # apply_to_graphs(g.ax, False, 0.285)
    # g.ax.set_xscale("log")
    g.ax.set_ylabel("")
    hatches = ["//", "..", "//", ".."]
    apply_hatch(g, patch_legend=True, hatch_list=hatches)

    FONT_SIZE = 9
    g.ax.annotate(
        "Lower is better",
        xycoords="axes points",
        xy=(0, 0),
        xytext=(1, -30),
        fontsize=FONT_SIZE,
        color="navy",
        weight="bold",
    )
    g.ax.annotate(
        "",
        xycoords="axes points",
        xy=(-15, -27),
        xytext=(0, -27),
        fontsize=FONT_SIZE,
        arrowprops=dict(arrowstyle="-|>", color="navy"),
    )

    g.despine()
    format(g.ax.xaxis, "kB")
    return g


def compute_ratio(x: pd.DataFrame) -> pd.Series:
    title = x.benchmark_title.iloc[0]
    scale = x.scale.iloc[0]
    native = x.value.iloc[0]
    if len(x.value) == 2:
        vmsh = x.value.iloc[1]
    else:
        print(f"WARNING: found only values for {title} for {x.identifier.iloc[0]}")
        # FIXME
        import math

        vmsh = math.nan
    if x.proportion.iloc[0] == "LIB":
        diff = vmsh / native
        proportion = "lower is better"
    else:
        diff = native / vmsh
        proportion = "higher is better"

    result = dict(
        title=x.title.iloc[0],
        benchmark_title=title,
        benchmark_group=x.benchmark_name,
        diff=diff,
        native=native,
        vmsh=vmsh,
        scale=scale,
        proportion=proportion,
    )
    return pd.Series(result, name="metrics")


CONVERSION_MAPPING = {
    "MB": 10e6,
    "KB": 10e3,
}

ALL_UNITS = "|".join(CONVERSION_MAPPING.keys())
UNIT_FINDER = re.compile(r"(\d+)\s*({})".format(ALL_UNITS), re.IGNORECASE)


def unit_replacer(matchobj: re.Match) -> str:
    """Given a regex match object, return a replacement string where units are modified"""
    number = matchobj.group(1)
    unit = matchobj.group(2)
    new_number = int(number) * CONVERSION_MAPPING[unit]
    return f"{new_number} B"


def sort_row(val: pd.Series) -> Any:
    return natsort_keygen()(val.apply(lambda v: UNIT_FINDER.sub(unit_replacer, v)))


def bar_colors(graph: Any, df: pd.Series, num_colors: int) -> None:
    colors = sns.color_palette(n_colors=num_colors)
    groups = 0
    last_group = df[0].iloc[0]
    for i, patch in enumerate(graph.axes[0][0].patches):
        if last_group != df[i].iloc[0]:
            last_group = df[i].iloc[0]
            groups += 1
        patch.set_facecolor(colors[groups])


def phoronix(df: pd.DataFrame) -> Any:
    df = df[df["identifier"].isin(["vmsh-blk", "qemu-blk"])]
    groups = len(df.benchmark_name.unique())
    # same benchmark with different units
    df = df[~((df.benchmark_name.str.startswith("pts/fio")) & (df.scale == "MB/s"))]
    df = df.sort_values(by=["benchmark_id", "identifier"], key=sort_row)
    df = df.groupby("benchmark_id").apply(compute_ratio).reset_index()
    df = df.sort_values(by=["benchmark_id"], key=sort_row)
    g = catplot(
        data=apply_aliases(df),
        y=column_alias("benchmark_id"),
        x=column_alias("diff"),
        kind="bar",
        palette=None,
    )
    bar_colors(g, df.benchmark_group, groups)
    g.ax.set_xlabel("")
    g.ax.set_ylabel("")
    FONT_SIZE = 9
    g.ax.annotate(
        "Lower is better",
        xycoords="axes fraction",
        xy=(0, 0),
        xytext=(0.1, -0.08),
        fontsize=FONT_SIZE,
        color="navy",
        weight="bold",
    )
    g.ax.annotate(
        "",
        xycoords="axes fraction",
        xy=(0.0, -0.07),
        xytext=(0.1, -0.07),
        fontsize=FONT_SIZE,
        arrowprops=dict(arrowstyle="-|>", color="navy"),
    )
    g.ax.axvline(x=1, color="gray", linestyle=":")
    g.ax.annotate(
        "baseline",
        xy=(1.1, -0.2),
        fontsize=FONT_SIZE,
    )
    return g


def sort_baseline_first(df: pd.DataFrame, baseline_system: str) -> pd.DataFrame:
    beginning = df[df["system"] == baseline_system]
    end = df[df["system"] != baseline_system]
    return pd.concat([beginning, end])


def nginx(df: pd.DataFrame, what: str) -> Any:
    # df = df[df["benchmark"] == what]
    df = df.melt(id_vars=["Unnamed: 0"], var_name="system", value_name="nginx-requests")
    df = parse_app_system(df)
    df = df[df["rootfs"] == "initrd"][df["app"] == what]
    df = sort(df, ["nginx_noshell_initrd_nohuman", "nginx_ushell_initrd_nohuman"])

    width = 3.3
    aspect = 2.0
    g = catplot(
        data=apply_aliases(df),
        y=column_alias("system"),
        # order=systems_order(df),
        x=column_alias("nginx-requests"),
        # hue=column_alias("direction"),
        kind="bar",
        ci="sd",  # show standard deviation! otherwise with_stddev_to_long_form does not work.
        height=width/aspect,
        aspect=aspect,
        palette=palette,
        legend=False,
        row="app",
        # sharex=True,
        # sharey=False,
        # facet_kws=dict({"gridspec_kws": {"height_ratios": [directs, files]}}),
    )
    # g.ax.set_xscale("log")

    FONT_SIZE = 9
    g.ax.annotate(
        "Higher is better",
        xycoords="axes points",
        xy=(0, 0),
        xytext=(-90, -30),
        fontsize=FONT_SIZE,
        color="navy",
        weight="bold",
    )
    g.ax.annotate(
        "",
        xycoords="axes points",
        xy=(-0, -27),
        xytext=(-15, -27),
        fontsize=FONT_SIZE,
        arrowprops=dict(arrowstyle="-|>", color="navy"),
    )

    return g


def redis(df: pd.DataFrame, what: str) -> Any:
    # df = df[df["benchmark"] == what]
    df = df.melt(id_vars=["Unnamed: 0"], var_name="system", value_name="redis-requests")
    df = parse_app_system(df)
    df = df[df["rootfs"] == "initrd"][df["app"] == what]
    df = sort_baseline_first(df, "redis_noshell_initrd_nohuman")
    # default font size seems to be 9, scaling it to around 10 crashes seaborn though
    # sns.set(font_scale=1.11)
    
    width = 2.5
    aspect = 1.5
    g = catplot(
        data=apply_aliases(df),
        y=column_alias("direction"),
        # order=systems_order(df),
        x=column_alias("redis-requests"),
        hue=column_alias("system"),
        kind="bar",
        ci="sd",  # show standard deviation! otherwise with_stddev_to_long_form does not work.
        height=width/aspect,
        aspect=aspect,
        palette=palette,
        legend=True,
        row="app",
        # fontsize=FONT_SIZE,
        # sharex=True,
        # sharey=False,
        # facet_kws=dict({"gridspec_kws": {"height_ratios": [directs, files]}}),
    )
    # g.ax.set_xlabel("foo", fontsize=10)
    # g.ax.set_xscale("log")
    apply_hatch(g, patch_legend=True, hatch_list=hatches)
    # sns.move_legend(g.ax, "upper center")
    # g.ax.legend(loc='upper center')

    # change_width(g.ax, 3.3)

    FONT_SIZE = 9
    g.ax.annotate(
        "Higher is better",
        xycoords="axes points",
        xy=(0, 0),
        xytext=(-30, -30),
        fontsize=FONT_SIZE,
        color="navy",
        weight="bold",
    )
    g.ax.annotate(
        "",
        xycoords="axes points",
        xy=(70, -27),
        xytext=(55, -27),
        fontsize=FONT_SIZE,
        arrowprops=dict(arrowstyle="-|>", color="navy"),
    )

    return g


def sqlite(df: pd.DataFrame, what: str) -> Any:
    # df = df[df["benchmark"] == what]
    df = df.melt(id_vars=["Unnamed: 0"], var_name="system", value_name="sqlite-seconds")
    df = parse_app_system(df)
    df = df[df["rootfs"] == "initrd"][df["app"] == what]
    df = sort_baseline_first(df, "sqlite_noshell_initrd_nohuman")

    width = 3.3
    aspect = 2.0
    g = catplot(
        data=apply_aliases(df),
        y=column_alias("system"),
        # order=systems_order(df),
        x=column_alias("sqlite-seconds"),
        # hue=column_alias("direction"),
        kind="bar",
        ci="sd",  # show standard deviation! otherwise with_stddev_to_long_form does not work.
        height=width/aspect,
        aspect=aspect,
        palette=palette,
        legend=False,
        row="app",
        # sharex=True,
        # sharey=False,
        # facet_kws=dict({"gridspec_kws": {"height_ratios": [directs, files]}}),
    )
    # g.ax.set_xscale("log")

    FONT_SIZE = 9
    g.ax.annotate(
        "Lower is better",
        xycoords="axes points",
        xy=(0, 0),
        xytext=(1, -30),
        fontsize=FONT_SIZE,
        color="navy",
        weight="bold",
    )
    g.ax.annotate(
        "",
        xycoords="axes points",
        xy=(-15, -27),
        xytext=(0, -27),
        fontsize=FONT_SIZE,
        arrowprops=dict(arrowstyle="-|>", color="navy"),
    )

    return g

def fio_overhead(df: pd.DataFrame, what: str, value_name: str) -> Any:
    df = df[df["benchmark"] == what]
    df = df.melt(
        id_vars=["system", "benchmark", "Unnamed: 0", "write_stddev", "read_stddev"],
        var_name="direction",
        value_name=value_name,
    )
    df = gobshit_to_stddev(df)
    df = system_to_iotype(df, value_name)
    warnings.simplefilter("ignore")
    fr = df[df.iotype == "file"][df.direction == "read_mean"]
    fw = df[df.iotype == "file"][df.direction == "write_mean"]
    dr = df[df.iotype == "direct"][df.direction == "read_mean"]
    dw = df[df.iotype == "direct"][df.direction == "write_mean"]
    warnings.simplefilter("default")

    def foo(g: pd.DataFrame, system: str) -> pd.DataFrame:
        df = pd.DataFrame()
        mean = float(g[g.system == system][value_name])
        stddev = float(g[g.system == system]["stddev"])

        def f(row: pd.DataFrame) -> pd.DataFrame:
            # row["stddev"] /= row[value_name]
            # if str(row.system) != system:
            #    from math import sqrt
            #    row["stddev"] = sqrt(pow(row["stddev"], 2) * pow(stddev/mean, 2))
            #    row["stddev"] *= stddev/mean
            # row[value_name] /= mean
            #
            # i dont think there actually exsists a stddev of this:
            # https://en.wikipedia.org/wiki/Ratio_distribution#Uncorrelated_central_normal_ratio
            row["stddev"] /= row[value_name]
            if str(row.system) != system:
                row["stddev"] += stddev / mean
            row[value_name] = mean / row[value_name]
            row["stddev"] *= row[value_name]
            #
            # first try:
            # row[value_name] /= mean
            # row["stddev"] += stddev
            # row["stddev"] /= mean
            return row

        g = g.apply(f, axis=1)
        df = df.append(g)
        return df

    fr = foo(fr, "detached_qemublk")
    fw = foo(fw, "detached_qemublk")
    dr = foo(dr, "direct_detached_qemublk")
    dw = foo(dw, "direct_detached_qemublk")
    df = pd.concat([dr, fr, dw, fw], sort=True)  # TODO fix sorting
    df = stddev_to_series(df, value_name, "stddev")
    directs = sum([int(t == "direct") for t in df["iotype"]])
    files = sum([int(t == "file") for t in df["iotype"]])
    g = catplot(
        data=apply_aliases(df),
        y=column_alias("system"),
        # order=systems_order(df),
        x=column_alias(value_name),
        hue=column_alias("direction"),
        kind="bar",
        ci="sd",  # show standard deviation! otherwise with_stddev_to_long_form does not work.
        height=2.3,
        aspect=2,
        # color=color,
        palette=None,
        legend=False,
        row="iotype",
        sharex=False,
        sharey=False,
        facet_kws=dict({"gridspec_kws": {"height_ratios": [directs, files]}}),
    )
    g.axes[0][0].legend(
        loc="upper right", frameon=True, title=column_alias("direction")
    )
    g.axes[1][0].set_xlabel("Overhead: " + g.axes[1][0].get_xlabel())
    g.axes[0][0].set_ylabel("")
    g.axes[1][0].set_ylabel("")
    g.axes[0][0].grid()
    g.axes[1][0].grid()
    return g


def main() -> None:
    if len(sys.argv) < 2:
        print(f"USAGE: {sys.argv[0]} graph.tsv...")
    graphs = []
    for arg in sys.argv[1:]:
        tsv_path = Path(arg)
        df = pd.read_csv(tsv_path, sep="\t")
        assert isinstance(df, pd.DataFrame)
        name = tsv_path.stem

        if name.startswith("app"):
            graphs.append(("nginx", nginx(df, "nginx")))
            graphs.append(("sqlite", sqlite(df, "sqlite")))
            graphs.append(("redis", redis(df, "redis")))
            graphs.append(("run", console(df, "ushell_run", names=["ushell_run", "ushell-run-cached"])))
        elif name.startswith("console"):
            graphs.append(("console", console(df, "ushell-console", names=["qemu_ssh_console", "ushell-console", "ushell-console-nginx"])))
            graphs.append(("init", console(df, "ushell-init")))
        elif name.startswith("image"):
            graphs.append(("images", images(df, "image-sizes")))
        else:
            print(f"unhandled graph name: {tsv_path}", file=sys.stderr)
            sys.exit(1)

    for prefix, graph in graphs:
        fname = f"{prefix}{out_format}"
        print(f"write {fname}")
        graph.savefig(fname)


if __name__ == "__main__":
    main()
