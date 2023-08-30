#!/usr/bin/env python3

from root import MEASURE_RESULTS

import pandas as pd
import re
import sys
from pathlib import Path
from typing import Any, List
from natsort import natsort_keygen
import warnings
import plot

from plot import (
    apply_aliases,
    catplot,
    column_alias,
    explode,
    sns,
    PAPER_MODE,
    plt,
    mpl,
    format,
    magnitude_formatter,
    change_width,
    apply_hatch,
    apply_hatch2,
    apply_hatch_ax,
)
from plot import ROW_ALIASES, COLUMN_ALIASES, FORMATTER

if PAPER_MODE:
    out_format = ".pdf"
else:
    out_format = ".png"

palette = sns.color_palette("pastel")
# palette = sns.color_palette("colorblind")
# palette = [palette[-1], palette[1], palette[2]]
palette_mpk = [palette[0], palette[2]]
col_base = palette[0]
col_ushell = palette[1]
col_ushellmpk = palette[2]

# hatches = ["", "..", "//"]
hatches = ["", "..", "**"]
hatches_mpk = ["", "**"]
hatch_base = hatches[0]
hatch_ushell = hatches[1]
hatch_ushellmpk = hatches[2]
barheight = 0.5
app_height = 1.8
#sysname = "ᴜSʜᴇʟʟ"
sysname = "ᴜIO"

ROW_ALIASES.update(
    {
        "system": {
            "ushell-console": f"{sysname}*",
            "ushellmpk-console": f"isolated-{sysname}",
            "ushell-console-nginx": f"{sysname} + Nginx load",
            "ushellmpk-console-nginx": f"isolated-{sysname} + Nginx load",
            "ushell-bpf-console": f"{sysname}*",
            "ushell-bpf-console-nginx": f"{sysname}* w/ Nginx load",
            "ushellmpk-bpf-console": f"{sysname}",
            "ushellmpk-bpf-console-nginx": f"{sysname} w/ Nginx load",
            "qemu_ssh_console": "linux + ssh",
            "ushell-init": "wo/ isolation",
            "redis_ushell_initrd_nohuman": f"{sysname}",
            "redis_ushellmpk_initrd_nohuman": f"isolated-{sysname}",
            "redis_noshell_initrd_nohuman": "Unikraft",
            "sqlite_ushell_initrd_nohuman": f"{sysname}",
            "sqlite_ushellmpk_initrd_nohuman": f"isolated-{sysname}",
            "sqlite_noshell_initrd_nohuman": "SQLite-Unikraft",
            "nginx_ushell_initrd_nohuman": f"{sysname}",
            "nginx_ushellmpk_initrd_nohuman": f"isolated-{sysname}",
            "nginx_noshell_initrd_nohuman": "Nginx-Unikraft",
            "nginx_ushell_initrd_lshuman": f"{sysname} interaction",
            "nginx_ushellmpk_initrd_lshuman": f"isolated-{sysname} interaction",
            "ushell_run": f"{sysname}",
            "ushellmpk_run": f"isolated-{sysname}",
            "ushell-run-cached": "cached",
            "ushellmpk-run-cached": "cached + isolated",
            "ushell-bpf_run": f"{sysname}*",
            "ushell-bpf-run-cached": f"{sysname}* (cached)",
            "ushellmpk-bpf_run": f"{sysname}",
            "ushellmpk-bpf-run-cached": f"{sysname} (cached)",
            "uk-count-noshell": f"count",
            "uk-count-ushell": f"count {sysname}",
            "uk-count-ushellmpk": f"count isolated-{sysname}",
            "uk-nginx-noshell-initrd": "Nginx",
            "uk-nginx-ushell-initrd": f"Nginx {sysname}",
            "uk-nginx-ushellmpk-initrd": f"Nginx isolated-{sysname}",
            "uk-redis-noshell-initrd": "Redis",
            "uk-redis-ushell-initrd": f"Redis {sysname}",
            "uk-redis-ushellmpk-initrd": f"Redis isolated-{sysname}",
            "uk-sqlite_benchmark-noshell-initrd": "SQLite",
            "uk-sqlite_benchmark-ushell-initrd": f"SQLite {sysname}",
            "uk-sqlite_benchmark-ushellmpk-initrd": f"SQLite isolated-{sysname}",
        },
        "ltoshell": {
            "noshell": "Unikraft",
            "noshell-lto": "Unikraft (opt)",
            "ushell": f"w/ {sysname}",
            "ushell-lto": f"w/ {sysname} (opt)",
            "ushellmpk": f"w/ isolated-{sysname}",
            "ushellmpk-lto": f"w/ isolated-{sysname} (opt)",
        },
        "ltoshell2": {
            "noshell-lto": "Unikraft",
            "ushellmpk-lto": f"{sysname}",
        },
        # "shell": {
            # "noshell": "baseline",
        # },
        "iotype": dict(
            direct="Direct/Block IO",
            file="File IO",
        ),
    }
)


COLUMN_ALIASES.update(
    {
        "ushell-console-seconds": "latency [us]",
        "ushell-init-seconds": "time [us]",
        "ushell_run-seconds": "time [us]",
        "redis-requests": "throughput [M req/s]",
        "nginx-requests": "throughput [K req/s]",
        "sqlite-seconds": "time [s]",
        "image-size": "size [kB]",
        # "ltoshell": "Variant",
        "system": "system"
    }
)
FORMATTER.update(
    {
        "iops": magnitude_formatter(3),
        "io_throughput": magnitude_formatter(6),
        "useconds": magnitude_formatter(-6),
        "kB": magnitude_formatter(3),
        "MB": magnitude_formatter(6),
        "krps": magnitude_formatter(3, offsetstring=True),
        "mrps": magnitude_formatter(6, offsetstring=True),
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

    # sns.set(font_scale=1.3)
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
            return "Redis"
        elif "nginx" in row.system:
            return "Nginx"
        elif "sqlite" in row.system:
            return "SQLite"
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
        elif "ushellmpk" in row.system:
            return "ushellmpk"
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


def sort(df: pd.DataFrame, systems: List[str], col = "system") -> pd.DataFrame:
    sparse = pd.concat([ df[df[col] == n] for n in systems ])
    return pd.concat([ sparse, df ]).drop_duplicates(keep='first')

def annotate_bar_values_s(g: Any):
    for c in g.ax.containers:
        labels = [f'   {(v.get_width()):.2f}s' for v in c]
        g.ax.bar_label(c, labels=labels, label_type='edge')

def annotate_bar_values_s2_ax(ax: Any, fontsize=7, rotation=0):
    for c in ax.containers:
        labels = [f'{(v.get_height()):.2f}s' for v in c]
        ax.bar_label(c, labels=labels, label_type='edge',
                     padding=2, fontsize=fontsize, rotation=rotation)

def annotate_bar_values_s2(g: Any):
    for c in g.ax.containers:
        labels = [f'{(v.get_height()):.2f}s' for v in c]
        g.ax.bar_label(c, labels=labels, label_type='edge',
                       padding=3, fontsize=5)

def annotate_bar_values_us(g: Any, offsets=None):
    for i, c in enumerate(g.ax.containers):
        if offsets is None:
            space = [' '*3]*len(c)
        else:
            space = [' '*s for s in offsets]
        labels = [f'{s}{(v.get_width()*1000*1000):.1f}us' for s,v in zip(space, c)]
        g.ax.bar_label(c, labels=labels, label_type='edge', fontsize=7)

def annotate_bar_values_k(g: Any):
    for c in g.ax.containers:
        labels = [f'   {(v.get_width()/1000):.1f}k' for v in c]
        g.ax.bar_label(c, labels=labels, label_type='edge')

def annotate_bar_values_kB(g: Any):
    for c in g.ax.containers:
        labels = [f' {(v.get_width()/1024/1024):.2f}MB' for v in c]
        g.ax.bar_label(c, labels=labels, label_type='edge', fontsize=7)

def annotate_bar_values_k_ax(ax: Any, fontsize=7, rotation=0):
    for c in ax.containers:
        labels = [f'{(v.get_height()/1000):.1f}k' for v in c]
        ax.bar_label(c, labels=labels, label_type='edge',
                     padding=3, fontsize=fontsize, rotation=rotation)

def annotate_bar_values_k2(g: Any):
    for c in g.ax.containers:
        labels = [f'{(v.get_height()/1000):.1f}k' for v in c]
        g.ax.bar_label(c, labels=labels, label_type='edge',
                       padding=3, fontsize=5)

def annotate_bar_values_M(g: Any):
    for c in g.ax.containers:
        labels = [f'   {(v.get_width()/1000/1000):.2f}M' for v in c]
        g.ax.bar_label(c, labels=labels, label_type='edge')

def annotate_bar_values_M2_ax(ax: Any, fontsize=7, rotation=0):
    for c in ax.containers:
        labels = [f'{(v.get_height()/1000/1000):.2f}M' for v in c]
        ax.bar_label(c, labels=labels, label_type='edge',
                     padding=3, fontsize=fontsize, rotation=90)

def annotate_bar_values_M2(g: Any):
    for c in g.ax.containers:
        labels = [f'{(v.get_height()/1000/1000):.2f}M' for v in c]
        g.ax.bar_label(c, labels=labels, label_type='edge', padding=3, fontsize=5)

def console(df: pd.DataFrame, name: str, aspect: float = 2.0, names: List[str] = []) -> Any:
    if len(names) == 0: names = [name]
    df = df.melt(id_vars=["Unnamed: 0"], var_name="system", value_name=f"{name}-seconds")
    df = pd.concat([ df[df["system"] == n] for n in names ])
    # df = df.append(dict(system=r"human", seconds=0.013), ignore_index=True)
    width = 3.3
    palette_console = [col_base, col_ushell, col_ushell, col_ushellmpk, col_ushellmpk]
    hatch_list_console = ["", "...", "|...", "**", "|**"]
    g = catplot(
        data=apply_aliases(df),
        y=column_alias("system"),
        # order=systems_order(df),
        x=column_alias(f"{name}-seconds"),
        kind="bar",
        ci="sd",  # show standard deviation! otherwise with_stddev_to_long_form does not work.
        height=width/aspect,
        aspect=aspect,
        palette=palette_console,
    )
    # plot.set_barplot_height(g.ax, barheight)
    # apply_to_graphs(g.ax, False, 0.285)
    # g.ax.set_xscale("log")
    g.ax.set_ylabel("")
    xytext = (-80,-27)
    # hatch_list_console = ["", "...", "**", "\\\\...", "\\\\**"]
    # hatch_list_run = ["...", "**", "/...", "/**"]
    hatch_list_run = ["...", "/...", "**", "/**"]
    offsets = None
    if name == "ushell-console": 
        offsets = [7, 3, 3, 3, 3]
        apply_hatch2(g, patch_legend=False, hatch_list=hatch_list_console)
        bar_colors(g, palette_console)
        # bar_colors(g, [col_base, col_ushell, col_ushellmpk, col_ushell, col_ushellmpk])
        # bar_colors(g, [col_base, col_ushell, col_ushellmpk, palette[6], palette[6]])
    if name == "ushell_run": 
        offsets = [7, 7, 7, 7]
        apply_hatch2(g, patch_legend=False, hatch_list=hatch_list_run)
        # bar_colors(g, [col_ushell, col_ushellmpk, palette[4], palette[5]])
        # bar_colors(g, [col_ushell, col_ushellmpk, col_ushell, col_ushellmpk])
        bar_colors(g, [col_ushell, col_ushell, col_ushellmpk, col_ushellmpk])
        xytext = (-60,-27)
    annotate_bar_values_us(g, offsets)

    FONT_SIZE = 9
    g.ax.annotate(
        "Lower is better",
        xycoords="axes points",
        xy=(0, 0),
        xytext=xytext,
        #xytext=(-100, -27),
        fontsize=FONT_SIZE,
        color="navy",
        weight="bold",
    )
    g.ax.annotate(
        "",
        xycoords="axes points",
        xy=tuple(x+y for x,y in zip(xytext, (-15,2))),
        xytext=tuple(x+y for x,y in zip(xytext, (0,2))),
        #xy=(-115, -25),
        #xytext=(-100, -25),
        fontsize=FONT_SIZE,
        arrowprops=dict(arrowstyle="-|>", color="navy"),
    )

    g.despine()
    format(g.ax.xaxis, "useconds")
    g.tight_layout()
    return g


def images(df: pd.DataFrame, name: str, names: List[str] = []) -> Any:
    if len(names) == 0: names = [name]
    df = df.melt(id_vars=["Unnamed: 0"], var_name="system", value_name=f"image-size")
    df = parse_app_system(df)
    df = df.sort_values(by="shell")

    # df = pd.concat([ df[df["system"] == n] for n in names ])
    # df = df.append(dict(system=r"human", seconds=0.013), ignore_index=True)
    # sns.set(font_scale=1.1)
    # width = 2.8
    width = 3.3
    aspect = 1.2
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
        palette=[col_base, col_base, col_ushell, col_ushell],
    )
    # apply_to_graphs(g.ax, False, 0.285)
    # g.ax.set_xscale("log")
    g.ax.set_ylabel("")
    g.ax.set_yticklabels(["count", "Nginx", "Redis", "SQLite"])
    # hatches = ["//", "..", "//|", "..|"]
    hatches = ["", "|", "..", "..|"]
    apply_hatch(g, patch_legend=True, hatch_list=hatches)
    annotate_bar_values_kB(g)
    g._legend.set_title("")
    sns.move_legend(g, "upper right", bbox_to_anchor=(0.77, 1.01), labelspacing=.2)
    # g.ax.set_xlim(0, 2500000)
    g.ax.set_xlim(0, 2800000)

    FONT_SIZE = 9
    g.ax.annotate(
        "Lower is better",
        xycoords="axes points",
        xy=(0, 0),
        xytext=(-20, -27),
        fontsize=FONT_SIZE,
        color="navy",
        weight="bold",
    )
    g.ax.annotate(
        "",
        xycoords="axes points",
        xy=(-20-15, -25),
        xytext=(-20, -25),
        fontsize=FONT_SIZE,
        arrowprops=dict(arrowstyle="-|>", color="navy"),
    )

    g.despine()
    format(g.ax.xaxis, "kB")
    return g

def images2(df: pd.DataFrame, names: List[str] = [], aspect: float = 2.0) -> Any:
    # Unikraft vs. ushell (opt version)

    df = df.melt(id_vars=["Unnamed: 0"], var_name="system", value_name=f"image-size")
    df = parse_app_system(df)
    df = df.sort_values(by="shell")

    if len(names) != 0:
        df = pd.concat([ df[df["system"] == n] for n in names ])
    print(df)

    width = 3.3
    g = catplot(
        data=apply_aliases(df),
        y=column_alias("app"),
        x=column_alias(f"image-size"),
        hue="shell",
        kind="bar",
        height=width/aspect,
        aspect=aspect,
        palette=[col_base, col_ushellmpk],
        legend=None,
    )
    # apply_to_graphs(g.ax, False, 0.285)
    # g.ax.set_xscale("log")
    g.ax.set_ylabel("")
    g.ax.set_yticklabels(["count", "Nginx", "Redis", "SQLite"])
    annotate_bar_values_kB(g)
    # sns.move_legend(g, "upper right", bbox_to_anchor=(0.77, 1.01), labelspacing=.2)
    g.ax.set_xlim(0, 2800000)

    hatches = ["", "**"]
    apply_hatch(g, patch_legend=False, hatch_list=hatches)
    p1 = mpl.patches.Patch(facecolor=col_base, hatch=hatch_base, edgecolor="k",
                           label='Unikraft')
    p2 = mpl.patches.Patch(facecolor=col_ushellmpk, hatch=hatch_ushellmpk, edgecolor="k",
                           label=f'{sysname}')
    g.ax.legend(handles=[p1, p2], title="", labelspacing=.2,
                loc="upper right", #bbox_to_anchor=(1.10, 1.05),
                fontsize=7, frameon=False)

    FONT_SIZE = 9
    g.ax.annotate(
        "Lower is better",
        xycoords="axes points",
        xy=(0, 0),
        xytext=(-20, -27),
        fontsize=FONT_SIZE,
        color="navy",
        weight="bold",
    )
    g.ax.annotate(
        "",
        xycoords="axes points",
        xy=(-20-15, -25),
        xytext=(-20, -25),
        fontsize=FONT_SIZE,
        arrowprops=dict(arrowstyle="-|>", color="navy"),
    )

    g.despine()
    format(g.ax.xaxis, "kB")
    return g


# plot both baseline and ushell
def memory(df: pd.DataFrame,
           names: List[str] = [],
           value_name="max_mem_use",
           aspect: float = 1.5) -> Any:
    df = df[df["Unnamed: 0"] == value_name]
    df = df.melt(id_vars=["Unnamed: 0"], var_name="system", value_name=value_name)
    df = parse_app_system(df)
    if len(names) != 0:
        df = pd.concat([ df[df["system"] == n] for n in names ])
    print(df)

    width = 3.3

    g = catplot(
        data=apply_aliases(df),
        y="app",
        x=value_name,
        kind="bar",
        hue="shell",
        height=width/aspect,
        aspect=aspect,
        palette=palette_mpk,
        legend=False,
    )

    g.ax.set_ylabel("")
    g.ax.set_xlabel("Maximum memory usage to boot [MB]")
    apply_hatch(g, patch_legend=False, hatch_list=hatches_mpk)
    annotate_bar_values_kB(g)

    # legend
    p1 = mpl.patches.Patch(facecolor=col_base, hatch=hatch_base, edgecolor="k",
                           label='Unikraft')
    p2 = mpl.patches.Patch(facecolor=col_ushellmpk, hatch=hatch_ushellmpk, edgecolor="k",
                           label=f'{sysname}')
    g.ax.legend(handles=[p1, p2], title="", labelspacing=.2,
                loc="upper right", bbox_to_anchor=(1.10, 1.05),
                fontsize=7, frameon=False)

    # annotation
    FONT_SIZE = 9
    xytext=(110, 10)
    g.ax.annotate(
        "Lower is better",
        xycoords="axes points",
        xy=(0, 0),
        xytext=xytext,
        fontsize=FONT_SIZE,
        color="navy",
        weight="bold",
    )
    g.ax.annotate(
        "",
        xycoords="axes points",
        xy=tuple(x+y for x,y in zip(xytext, (-15,2))),
        xytext=tuple(x+y for x,y in zip(xytext, (0,2))),
        fontsize=FONT_SIZE,
        arrowprops=dict(arrowstyle="-|>", color="navy"),
    )

    g.despine()
    format(g.ax.xaxis, "MB")
    g.tight_layout()
    return g

# plot memory increase in the unikernels or the host
def memory2(df: pd.DataFrame,
            names: List[str],
            baseline: List[str],
            value_name = "max_mem_use",
            aspect: float = 1.5,
            ) -> Any:

    def get_df(value_name):
        df1 = df[df["Unnamed: 0"] == value_name]
        df1 = df1.melt(id_vars=["Unnamed: 0"], var_name="system", value_name=value_name)
        df1_base = pd.concat([ df1[df1["system"] == n] for n in baseline ]).reset_index(drop=True)
        df1 = pd.concat([ df1[df1["system"] == n] for n in names ]).reset_index(drop=True)
        df1[value_name] -= df1_base[value_name]
        return df1.drop("Unnamed: 0", axis=1)

    df1 = get_df("max_mem_use")
    df2 = get_df("total_host_mem_with_shell_peak")
    df = df1.merge(df2)
    df = parse_app_system(df)
    print(df)

    width = 3.3

    g = catplot(
        data=apply_aliases(df),
        y="app",
        x=value_name,
        kind="bar",
        hue="shell",
        height=width/aspect,
        aspect=aspect,
        palette=[palette[2]],
        legend=False,
    )

    g.ax.set_ylabel("")
    g.ax.set_xlabel("Maximum memory usage increase [MB]")
    apply_hatch(g, patch_legend=False, hatch_list=[hatches[2]])
    annotate_bar_values_kB(g)

    # legend
    # p1 = mpl.patches.Patch(facecolor=col_ushellmpk, hatch=hatch_ushellmpk, edgecolor="k",
    #                        label=f'{sysname}')
    # g.ax.legend(handles=[p1], title="", labelspacing=.2,
    #             loc="upper right", bbox_to_anchor=(1.10, 1.05),
    #             fontsize=7, frameon=False)

    # annotation
    FONT_SIZE = 9
    xytext=(110, 10)
    g.ax.annotate(
        "Lower is better",
        xycoords="axes points",
        xy=(0, 0),
        xytext=xytext,
        fontsize=FONT_SIZE,
        color="navy",
        weight="bold",
    )
    g.ax.annotate(
        "",
        xycoords="axes points",
        xy=tuple(x+y for x,y in zip(xytext, (-15,2))),
        xytext=tuple(x+y for x,y in zip(xytext, (0,2))),
        fontsize=FONT_SIZE,
        arrowprops=dict(arrowstyle="-|>", color="navy"),
    )

    g.despine()
    format(g.ax.xaxis, "MB")
    g.tight_layout()
    return g

# plot memory increase in the unikernel and the host side
def memory3(df: pd.DataFrame,
            names: List[str],
            baseline: List[str],
            aspect: float = 2.0,
            ) -> Any:

    def get_df(value_name):
        df1 = df[df["Unnamed: 0"] == value_name]
        df1 = df1.melt(id_vars=["Unnamed: 0"], var_name="system", value_name=value_name)
        df1_base = pd.concat([ df1[df1["system"] == n] for n in baseline ]).reset_index(drop=True)
        df1 = pd.concat([ df1[df1["system"] == n] for n in names ]).reset_index(drop=True)
        df1[value_name] -= df1_base[value_name]
        return df1.drop("Unnamed: 0", axis=1)

    df1 = get_df("max_mem_use")
    df2 = get_df("total_host_mem_with_shell_peak")
    df1 = df1.melt(id_vars=["system"], value_vars=["max_mem_use"], var_name="mem")
    df2 = df2.melt(id_vars=["system"], value_vars=["total_host_mem_with_shell_peak"], var_name="mem")
    df = pd.concat([df1, df2])
    df = parse_app_system(df)
    df = sort(df, ["count", "Nginx", "Redis", "SQLite"], "app")
    print(df)

    width = 3.3

    g = catplot(
        data=apply_aliases(df),
        y="app",
        x="value",
        kind="bar",
        hue="mem",
        height=width/aspect,
        aspect=aspect,
        palette=[palette[2]],
        legend=False,
    )

    g.ax.set_ylabel("")
    g.ax.set_xlabel("Increase in maximum memory usage [MB]")
    apply_hatch(g, patch_legend=False, hatch_list=[hatches[2]])
    annotate_bar_values_kB(g)

    # legend
    p1 = mpl.patches.Patch(facecolor=col_ushellmpk, hatch=hatch_ushellmpk, edgecolor="k",
                           label=f'{sysname}')
    p2 = mpl.patches.Patch(facecolor=col_ushellmpk, hatch="", edgecolor="k",
                           label=f'Host/guest total')
    g.ax.legend(handles=[p1, p2], title="", labelspacing=.2,
                loc="upper right", bbox_to_anchor=(1.10, 0.65),
                fontsize=7, frameon=False)

    # annotation
    FONT_SIZE = 9
    xytext=(110, 15)
    g.ax.annotate(
        "Lower is better",
        xycoords="axes points",
        xy=(0, 0),
        xytext=xytext,
        fontsize=FONT_SIZE,
        color="navy",
        weight="bold",
    )
    g.ax.annotate(
        "",
        xycoords="axes points",
        xy=tuple(x+y for x,y in zip(xytext, (-15,2))),
        xytext=tuple(x+y for x,y in zip(xytext, (0,2))),
        fontsize=FONT_SIZE,
        arrowprops=dict(arrowstyle="-|>", color="navy"),
    )

    g.despine()
    format(g.ax.xaxis, "MB")
    g.tight_layout()
    return g


def compute_ratio(x: pd.DataFrame) -> pd.Series:
    title = x.benchmark_title.iloc[0]
    scale = x.scale.iloc[0]
    native = x.value.iloc[0]
    if len(x.value) == 2:
        l = x.value.iloc[1]
    else:
        print(f"WARNING: found only values for {title} for {x.identifier.iloc[0]}")
        # FIXME
        import math

        l = math.nan
    if x.proportion.iloc[0] == "LIB":
        diff = l / native
        proportion = "lower is better"
    else:
        diff = native / l
        proportion = "higher is better"

    result = dict(
        title=x.title.iloc[0],
        benchmark_title=title,
        benchmark_group=x.benchmark_name,
        diff=diff,
        native=native,
        we=l,
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


def bar_colors_rainbow(graph: Any, df: pd.Series, num_colors: int) -> None:
    colors = sns.color_palette(n_colors=num_colors)
    groups = 0
    last_group = df[0].iloc[0]
    for i, patch in enumerate(graph.axes[0][0].patches):
        if last_group != df[i].iloc[0]:
            last_group = df[i].iloc[0]
            groups += 1
        patch.set_facecolor(colors[groups])


def bar_colors(graph: Any, colors) -> None:
    for i, patch in enumerate(graph.axes[0][0].patches):
        patch.set_facecolor(colors[i%len(colors)])


def sort_baseline_first(df: pd.DataFrame, baseline_system: str) -> pd.DataFrame:
    beginning = df[df["system"] == baseline_system]
    end = df[df["system"] != baseline_system]
    return pd.concat([beginning, end])


def nginx(df: pd.DataFrame, what: str) -> Any:
    # df = df[df["benchmark"] == what]
    df = df.melt(id_vars=["Unnamed: 0"], var_name="system", value_name="nginx-requests")
    df = parse_app_system(df)
    df = df[df["rootfs"] == "initrd"][df["app"] == what]
    names = [ "nginx_noshell_initrd_nohuman", "nginx_ushell_initrd_nohuman", "nginx_ushellmpk_initrd_nohuman"]
    df = pd.concat([ df[df["system"] == n] for n in names ])
    df = sort(df, ["nginx_noshell_initrd_nohuman", "nginx_ushell_initrd_nohuman"])

    width = (7/4)+0.2
    aspect = width/app_height
    g = catplot(
        data=apply_aliases(df),
        x=column_alias("system"),
        # order=systems_order(df),
        y=column_alias("nginx-requests"),
        # hue=column_alias("direction"),
        kind="bar",
        ci="sd",  # show standard deviation! otherwise with_stddev_to_long_form does not work.
        height=app_height,
        aspect=aspect,
        # aspect=1.1,
        palette=palette,
        legend=False,
        # row="app",
        # sharex=True,
        # sharey=False,
        # facet_kws=dict({"gridspec_kws": {"height_ratios": [directs, files]}}),
    )
    # g.ax.set_xscale("log")

    # plot.set_barplot_height(g.ax, barheight)
    annotate_bar_values_k2(g)
    g.despine()
    g.set(xticklabels=[], xlabel="")
    apply_hatch2(g, patch_legend=False, hatch_list=hatches)
    format(g.ax.yaxis, "krps")

    FONT_SIZE = 9
    g.ax.annotate(
        "Higher is better ↑",
        xycoords="axes points",
        # xy=(0, 0),
        xy=(0, 0),
        xytext=(-35, -15),
        fontsize=FONT_SIZE,
        color="navy",
        weight="bold",
    )
    # g.ax.annotate(
    #     "",
    #     xycoords="axes points",
    #     xy=(-20, 0),
    #     xytext=(-20, -20),
    #     fontsize=FONT_SIZE,
    #     arrowprops=dict(arrowstyle="-|>", color="navy"),
    # )

    return g


def redis(df: pd.DataFrame, what: str) -> Any:
    # df = df[df["benchmark"] == what]
    df = df.melt(id_vars=["Unnamed: 0"], var_name="system", value_name="redis-requests")
    df = parse_app_system(df)
    df = df[df["rootfs"] == "initrd"][df["app"] == what]
    df = sort_baseline_first(df, "redis_noshell_initrd_nohuman")
    # default font size seems to be 9, scaling it to around 10 crashes seaborn though
    # sns.set(font_scale=1.11)
    
    # width = 2.5
    # aspect = 1.5
    width = ((7/4)*2)*0.7
    aspect = width/app_height
    g = catplot(
        data=apply_aliases(df),
        x=column_alias("direction"),
        # order=systems_order(df),
        y=column_alias("redis-requests"),
        hue=column_alias("system"),
        kind="bar",
        ci="sd",  # show standard deviation! otherwise with_stddev_to_long_form does not work.
        height=app_height,
        aspect=aspect,
        palette=palette,
        legend=True,
        # row="app",
        capsize=0.1,
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
    # plot.set_barplot_height(g.ax, barheight)
    annotate_bar_values_M2(g)
    g.despine()
    g._legend.set_title("")
    g.ax.set_xlabel("")
    format(g.ax.yaxis, "mrps")

    # change_width(g.ax, 3.3)

    FONT_SIZE = 9
    g.ax.annotate(
        "Higher is better ↑",
        xycoords="axes points",
        xy=(0, 0),
        xytext=(100, -15),
        fontsize=FONT_SIZE,
        color="navy",
        weight="bold",
    )
    # g.ax.annotate(
    #     "",
    #     xycoords="axes points",
    #     xy=(70, -27),
    #     xytext=(55, -27),
    #     fontsize=FONT_SIZE,
    #     arrowprops=dict(arrowstyle="-|>", color="navy"),
    # )

    return g

def sqlite(df: pd.DataFrame, what: str) -> Any:
    # df = df[df["benchmark"] == what]
    df = df.melt(id_vars=["Unnamed: 0"], var_name="system", value_name="sqlite-seconds")
    df = parse_app_system(df)
    df = df[df["rootfs"] == "initrd"][df["app"] == what]
    df = sort_baseline_first(df, "sqlite_noshell_initrd_nohuman")

    # width = 3.3
    # aspect = 2.9
    width = (7/4)-0.2 # give some width to a nginx graph
    aspect = width/app_height
    g = catplot(
        data=apply_aliases(df),
        x=column_alias("system"),
        # order=systems_order(df),
        y=column_alias("sqlite-seconds"),
        # hue=column_alias("direction"),
        kind="bar",
        ci="sd",  # show standard deviation! otherwise with_stddev_to_long_form does not work.
        height=app_height,
        aspect=aspect,
        palette=palette,
        legend=False,
        # row="app",
        # sharex=True,
        # sharey=False,
        # facet_kws=dict({"gridspec_kws": {"height_ratios": [directs, files]}}),
    )
    # g.ax.set_xscale("log")
    annotate_bar_values_s2(g)
    g.despine()
    # plot.set_barplot_height(g.ax, barheight)
    g.set(xticklabels=[], xlabel="")
    apply_hatch2(g, patch_legend=False, hatch_list=hatches)

    FONT_SIZE = 9
    g.ax.annotate(
        "Lower is better ↓",
        xycoords="axes points",
        xy=(0, 0),
        xytext=(-30, -15),
        fontsize=FONT_SIZE,
        color="navy",
        weight="bold",
    )
    # g.ax.annotate(
    #     "",
    #     xycoords="axes points",
    #     xy=(-15, -27),
    #     xytext=(0, -27),
    #     fontsize=FONT_SIZE,
    #     arrowprops=dict(arrowstyle="-|>", color="navy"),
    # )

    return g


def app(df: pd.DataFrame) -> Any:

    width = 7 # \textwidth is 7 inch
    height = 1.8

    fig, axs = plt.subplots(ncols=3, gridspec_kw={'width_ratios': [1, 1, 2]})
    fig.set_size_inches(width, height)

    def plot_nginx(df, ax, what="nginx", fontsize=7):
        df = df.melt(id_vars=["Unnamed: 0"], var_name="system", value_name="nginx-requests")
        df = parse_app_system(df)
        df = df[df["rootfs"] == "initrd"][df["app"] == what]
        names = [ "nginx_noshell_initrd_nohuman", "nginx_ushell_initrd_nohuman", "nginx_ushellmpk_initrd_nohuman"]
        df = pd.concat([ df[df["system"] == n] for n in names ])
        df = sort(df, ["nginx_noshell_initrd_nohuman", "nginx_ushell_initrd_nohuman"])
        g = sns.barplot(
            ax=ax,
            data=apply_aliases(df),
            x=column_alias("system"),
            y=column_alias("nginx-requests"),
            ci="sd",
            palette=palette,
            edgecolor="k",
            errcolor="black",
            errwidth=1,
            capsize=0.2,
            # height=height,
            # aspect=nginx_width/height,
        )
        annotate_bar_values_k_ax(ax, fontsize)
        sns.despine(ax=ax)
        apply_hatch_ax(ax, patch_legend=False, hatch_list=hatches)
        format(ax.yaxis, "krps")
        g.set(xticks=[], xticklabels=[], xlabel="(a) Nginx")
        g.set_title("Higher is better ↑", fontsize=9, color="navy", weight="bold",
                    x = 0.40, y=1, pad=10)
        change_width(g, 0.8)
        return g

    def plot_sqlite(df, ax, what="sqlite", fontsize=7):
        df = df.melt(id_vars=["Unnamed: 0"], var_name="system", value_name="sqlite-seconds")
        df = parse_app_system(df)
        df = df[df["rootfs"] == "initrd"][df["app"] == what]
        df = sort_baseline_first(df, "sqlite_noshell_initrd_nohuman")

        g = sns.barplot(
            ax=ax,
            data=apply_aliases(df),
            x=column_alias("system"),
            y=column_alias("sqlite-seconds"),
            ci="sd",  # show standard deviation! otherwise with_stddev_to_long_form does not work.
            palette=palette,
            edgecolor="k",
            errcolor="black",
            errwidth=1,
            capsize=0.2,
        )

        annotate_bar_values_s2_ax(g, fontsize)
        sns.despine(ax=ax)
        g.set(xticks=[], xticklabels=[], xlabel="(b) SQLite")
        g.set_title("Lower is better ↓", fontsize=9, color="navy", weight="bold",
                    x = 0.45, y=1, pad=10)
        apply_hatch_ax(ax, patch_legend=True, hatch_list=hatches)
        change_width(g, 0.8)

    def plot_redis(df, ax, what="redis", fontsize=7):
        df = df.melt(id_vars=["Unnamed: 0"], var_name="system", value_name="redis-requests")
        df = parse_app_system(df)
        df = df[df["rootfs"] == "initrd"][df["app"] == what]
        df = sort_baseline_first(df, "redis_noshell_initrd_nohuman")

        g = sns.barplot(
            ax=ax,
            data=apply_aliases(df),
            x=column_alias("direction"),
            y=column_alias("redis-requests"),
            hue=column_alias("system"),
            ci="sd",  # show standard deviation! otherwise with_stddev_to_long_form does not work.
            palette=palette,
            edgecolor="k",
            errcolor="black",
            errwidth=1.0,
            # capsize=0.2,
            capsize=0.05,
        )
        annotate_bar_values_M2_ax(g, fontsize=fontsize)
        sns.despine(ax=ax)
        hatches = ["", "", "..", "..", "**", "**"]
        for idx, bar in enumerate(g.patches):
            bar.set_hatch(hatches[idx%len(hatches)])

        bar_width = 0.22
        change_width(g, bar_width)
        # reduce space between "get" and "set"
        # for idx, bar in enumerate(g.patches):
        #     if idx % 2 != 0:
        #         bar.set_x(bar.get_x() - 0.1)
        g.margins(x=0.001)
        g.set_xlabel("(c) Redis")
        g.tick_params(axis='x', length=0) # remove ticks
        g.set_title("Higher is better ↑", fontsize=9, color="navy", weight="bold", pad=10)
        format(ax.yaxis, "mrps")
        ax.legend([], [], frameon=False) # remove legend
        # g.legend(frameon=False) # (re-)draw legend with hatches
        # sns.move_legend(g, "center left", bbox_to_anchor=(1.00, 0.5))

    fs = 7 # font size of the valeus top of the bars
    plot_nginx(df, axs[0], fontsize=fs)
    plot_sqlite(df, axs[1], fontsize=fs)
    plot_redis(df, axs[2], fontsize=fs)

    # legend
    p1 = mpl.patches.Patch(facecolor=palette[0], hatch=hatches[0], edgecolor="k",
                           label='Unikraft')
    p2 = mpl.patches.Patch(facecolor=palette[1], hatch=hatches[1], edgecolor="k",
                           label=f'{sysname}')
    p3 = mpl.patches.Patch(facecolor=palette[2], hatch=hatches[2], edgecolor="k",
                           label=f'isolated-{sysname}')
    # fig.legend(handles=[p1, p2, p3], loc="center left", bbox_to_anchor=(1.0, 0.5), frameon=False, ncol=1)
    fig.legend(handles=[p1, p2, p3], loc="lower center", bbox_to_anchor=(0.5, -0.1), frameon=False, ncol=3)

    fig.tight_layout()

    return fig

def app2(df: pd.DataFrame, config_names, aspect:float = 4.0) -> Any:

    # this figure uses double column
    width = 7 # \textwidth is 7 inch
    # height = 1.8
    height = width/aspect

    fig, axs = plt.subplots(ncols=3, gridspec_kw={'width_ratios': [1, 1, 2]})
    fig.set_size_inches(width, height)

    def plot_nginx(df, ax, what="nginx", fontsize=7):
        df = df.melt(id_vars=["Unnamed: 0"], var_name="system", value_name="nginx-requests")
        df = parse_app_system(df)
        # df = df[df["rootfs"] == "initrd"][df["app"] == what]
        names = [ config_name.format(app=what) for config_name in config_names ]
        df = pd.concat([ df[df["system"] == n] for n in names ])
        df = sort(df, names)
        g = sns.barplot(
            ax=ax,
            data=apply_aliases(df),
            x=column_alias("system"),
            y=column_alias("nginx-requests"),
            ci="sd",
            palette=palette,
            edgecolor="k",
            errcolor="black",
            errwidth=1,
            capsize=0.2,
            # height=height,
            # aspect=nginx_width/height,
        )
        annotate_bar_values_k_ax(ax, fontsize)
        sns.despine(ax=ax)
        apply_hatch_ax(ax, patch_legend=False, hatch_list=hatches)
        format(ax.yaxis, "krps")
        g.set(xticks=[], xticklabels=[], xlabel="(a) Nginx")
        g.set_title("Higher is better ↑", fontsize=9, color="navy", weight="bold",
                    x = 0.40, y=1, pad=10)
        change_width(g, 0.8)
        return g

    def plot_sqlite(df, ax, what="sqlite", fontsize=7):
        df = df.melt(id_vars=["Unnamed: 0"], var_name="system", value_name="sqlite-seconds")
        df = parse_app_system(df)
        # df = df[df["rootfs"] == "initrd"][df["app"] == what]
        names = [ config_name.format(app=what) for config_name in config_names ]
        df = pd.concat([ df[df["system"] == n] for n in names ])
        df = sort(df, names)
        print(df)

        g = sns.barplot(
            ax=ax,
            data=apply_aliases(df),
            x=column_alias("system"),
            y=column_alias("sqlite-seconds"),
            ci="sd",  # show standard deviation! otherwise with_stddev_to_long_form does not work.
            palette=palette,
            edgecolor="k",
            errcolor="black",
            errwidth=1,
            capsize=0.2,
        )

        annotate_bar_values_s2_ax(g, fontsize)
        sns.despine(ax=ax)
        g.set(xticks=[], xticklabels=[], xlabel="(b) SQLite")
        g.set_title("Lower is better ↓", fontsize=9, color="navy", weight="bold",
                    x = 0.45, y=1, pad=10)
        apply_hatch_ax(ax, patch_legend=True, hatch_list=hatches)
        change_width(g, 0.8)

    def plot_redis(df, ax, what="redis", fontsize=7):
        df = df.melt(id_vars=["Unnamed: 0"], var_name="system", value_name="redis-requests")
        df = parse_app_system(df)
        names = [ config_name.format(app=what) for config_name in config_names ]
        df = pd.concat([ df[df["system"] == n] for n in names ])
        df = sort(df, names)


        g = sns.barplot(
            ax=ax,
            data=apply_aliases(df),
            x=column_alias("direction"),
            y=column_alias("redis-requests"),
            hue=column_alias("system"),
            ci="sd",  # show standard deviation! otherwise with_stddev_to_long_form does not work.
            palette=palette,
            edgecolor="k",
            errcolor="black",
            errwidth=1.0,
            # capsize=0.2,
            capsize=0.05,
        )
        annotate_bar_values_M2_ax(g, fontsize=fontsize)
        sns.despine(ax=ax)
        hatches = ["", "", "..", "..", "**", "**"]
        for idx, bar in enumerate(g.patches):
            bar.set_hatch(hatches[idx%len(hatches)])

        bar_width = 0.22
        change_width(g, bar_width)
        # reduce space between "get" and "set"
        # for idx, bar in enumerate(g.patches):
        #     if idx % 2 != 0:
        #         bar.set_x(bar.get_x() - 0.1)
        g.margins(x=0.001)
        g.set_xlabel("(c) Redis")
        g.tick_params(axis='x', length=0) # remove ticks
        g.set_title("Higher is better ↑", fontsize=9, color="navy", weight="bold", pad=10)
        format(ax.yaxis, "mrps")
        ax.legend([], [], frameon=False) # remove legend
        # g.legend(frameon=False) # (re-)draw legend with hatches
        # sns.move_legend(g, "center left", bbox_to_anchor=(1.00, 0.5))

    fs = 7 # font size of the valeus top of the bars
    plot_nginx(df, axs[0], fontsize=fs)
    plot_sqlite(df, axs[1], fontsize=fs)
    plot_redis(df, axs[2], fontsize=fs)

    # legend
    p1 = mpl.patches.Patch(facecolor=palette[0], hatch=hatches[0], edgecolor="k",
                           label='Unikraft')
    p2 = mpl.patches.Patch(facecolor=palette[1], hatch=hatches[1], edgecolor="k",
                           label=f'{sysname}*')
    p3 = mpl.patches.Patch(facecolor=palette[2], hatch=hatches[2], edgecolor="k",
                           label=f'{sysname}')
    # fig.legend(handles=[p1, p2, p3], loc="center left", bbox_to_anchor=(1.0, 0.5), frameon=False, ncol=1)
    fig.legend(handles=[p1, p2, p3], loc="lower center", bbox_to_anchor=(0.5, -0.1), frameon=False, ncol=3)

    fig.tight_layout()

    return fig

def app3(df: pd.DataFrame, config_names, aspect:float = 4.0) -> Any:

    # this figure uses double column
    width = 7 # \textwidth is 7 inch
    height = width/aspect

    fig, axs = plt.subplots(ncols=3, gridspec_kw={'width_ratios': [1, 2, 0.8]})
    # fig, axs = plt.subplots(ncols=2, gridspec_kw={'width_ratios': [1,  2]})
    fig.set_size_inches(width, height)

    hatches = ["", "...", "**", "**", "**"]

    def plot_nginx(df, ax, what="nginx", fontsize=7):
        df = df.melt(id_vars=["Unnamed: 0"], var_name="system", value_name="nginx-requests")
        df = parse_app_system(df)
        # df = df[df["rootfs"] == "initrd"][df["app"] == what]
        names = [ config_name.format(app=what) for config_name in config_names ]
        df = pd.concat([ df[df["system"] == n] for n in names ])
        df = sort(df, names)
        g = sns.barplot(
            ax=ax,
            data=apply_aliases(df),
            x=column_alias("system"),
            y=column_alias("nginx-requests"),
            ci="sd",
            palette=palette,
            edgecolor="k",
            errcolor="black",
            errwidth=1,
            capsize=0.2,
            # height=height,
            # aspect=nginx_width/height,
        )
        annotate_bar_values_k_ax(ax, fontsize)
        sns.despine(ax=ax)
        apply_hatch_ax(ax, patch_legend=False, hatch_list=hatches)
        format(ax.yaxis, "krps")
        g.set(xticks=[], xticklabels=[], xlabel="(a) Nginx")
        g.set_title("Higher is better ↑", fontsize=9, color="navy", weight="bold",
                    x = 0.40, y=1, pad=10)
        change_width(g, 0.8)
        return g

    def plot_sqlite(df, ax, what="sqlite", fontsize=7):
        df = df.melt(id_vars=["Unnamed: 0"], var_name="system", value_name="sqlite-seconds")
        df = parse_app_system(df)
        # df = df[df["rootfs"] == "initrd"][df["app"] == what]
        # sqlite does not have experiments with ls/perf/bpf
        names = [ config_name.format(app=what) for config_name in config_names[:3] ]
        df = pd.concat([ df[df["system"] == n] for n in names ])
        df = sort(df, names)
        print(df)

        g = sns.barplot(
            ax=ax,
            data=apply_aliases(df),
            x=column_alias("system"),
            y=column_alias("sqlite-seconds"),
            ci="sd",  # show standard deviation! otherwise with_stddev_to_long_form does not work.
            palette=palette,
            edgecolor="k",
            errcolor="black",
            errwidth=1,
            capsize=0.2,
        )

        annotate_bar_values_s2_ax(g, fontsize)
        sns.despine(ax=ax)
        g.set(xticks=[], xticklabels=[], xlabel="(b) SQLite")
        g.set_title("Lower is better ↓", fontsize=9, color="navy", weight="bold",
                    x = 0.45, y=1, pad=10)
        apply_hatch_ax(ax, patch_legend=True, hatch_list=hatches)
        change_width(g, 0.8)

    def plot_redis(df, ax, what="redis", fontsize=7):
        df = df.melt(id_vars=["Unnamed: 0"], var_name="system", value_name="redis-requests")
        df = parse_app_system(df)
        names = [ config_name.format(app=what) for config_name in config_names ]
        df = pd.concat([ df[df["system"] == n] for n in names ])
        df = sort(df, names)


        g = sns.barplot(
            ax=ax,
            data=apply_aliases(df),
            x=column_alias("direction"),
            y=column_alias("redis-requests"),
            hue=column_alias("system"),
            ci="sd",  # show standard deviation! otherwise with_stddev_to_long_form does not work.
            palette=palette,
            edgecolor="k",
            errcolor="black",
            errwidth=1.0,
            # capsize=0.2,
            capsize=0.05,
        )
        annotate_bar_values_M2_ax(g, fontsize=fontsize)
        sns.despine(ax=ax)
        hatches = ["", "", "..", "..", "**", "**", "**", "**", "**", "**"]
        for idx, bar in enumerate(g.patches):
            bar.set_hatch(hatches[idx%len(hatches)])

        bar_width = 0.10
        change_width(g, bar_width)
        # reduce space between "get" and "set"
        # for idx, bar in enumerate(g.patches):
        #     if idx % 2 != 0:
        #         bar.set_x(bar.get_x() - 0.1)
        g.margins(x=0.001)
        g.set_xlabel("(c) Redis")
        g.tick_params(axis='x', length=0) # remove ticks
        g.set_title("Higher is better ↑", fontsize=9, color="navy", weight="bold", pad=10)
        format(ax.yaxis, "mrps")
        ax.legend([], [], frameon=False) # remove legend
        # g.legend(frameon=False) # (re-)draw legend with hatches
        # sns.move_legend(g, "center left", bbox_to_anchor=(1.00, 0.5))

    fs = 7 # font size of the valeus top of the bars
    plot_nginx(df, axs[0], fontsize=fs)
    plot_redis(df, axs[1], fontsize=fs)
    plot_sqlite(df, axs[2], fontsize=fs)

    # legend
    handles = []
    handles.append(mpl.patches.Patch(facecolor=palette[0], hatch=hatches[0], edgecolor="k", label='Unikraft'))
    handles.append(mpl.patches.Patch(facecolor=palette[1], hatch=hatches[1], edgecolor="k", label=f'{sysname}*'))
    handles.append(mpl.patches.Patch(facecolor=palette[2], hatch=hatches[2], edgecolor="k", label=f'{sysname}'))
    handles.append(mpl.patches.Patch(facecolor=palette[3], hatch=hatches[2], edgecolor="k", label=f'{sysname} w/ ls'))
    handles.append(mpl.patches.Patch(facecolor=palette[4], hatch=hatches[2], edgecolor="k", label=f'{sysname} w/ perf'))
    fig.legend(handles=handles, loc="lower center", bbox_to_anchor=(0.5, -0.1), frameon=False, ncol=len(handles))

    fig.tight_layout()

    return fig

def app4(df: pd.DataFrame, config_names, aspect:float = 4.0) -> Any:

    # this figure uses double column
    width = 7 # \textwidth is 7 inch
    height = width/aspect

    fig, axs = plt.subplots(ncols=3, gridspec_kw={'width_ratios': [1, 2, 0.5]})
    # fig, axs = plt.subplots(ncols=2, gridspec_kw={'width_ratios': [1,  2]})
    fig.set_size_inches(width, height)

    # hatches = ["", "...", "**", "...", "**", "...", "**", "...", "**", "...", "**"]
    hatches = ["", "..", "**", "//..", "//**", "-..", "-**", "|..", "|**", "x..", "x**"]
    app_palette = [col_base, col_ushell, col_ushellmpk, palette[3], palette[3], palette[4], palette[4], palette[5], palette[5]]
    # app_palette = [col_base, col_ushell, col_ushellmpk, col_ushell, col_ushellmpk, col_ushell, col_ushellmpk, col_ushell, col_ushellmpk]

    title_pad = 37

    def plot_nginx(df, ax, what="nginx", fontsize=7):
        df = df.melt(id_vars=["Unnamed: 0"], var_name="system", value_name="nginx-requests")
        df = parse_app_system(df)
        # df = df[df["rootfs"] == "initrd"][df["app"] == what]
        names = [ config_name.format(app=what) for config_name in config_names ]
        df = pd.concat([ df[df["system"] == n] for n in names ])
        df = sort(df, names)
        g = sns.barplot(
            ax=ax,
            data=apply_aliases(df),
            x=column_alias("system"),
            y=column_alias("nginx-requests"),
            ci="sd",
            palette=app_palette,
            edgecolor="k",
            errcolor="black",
            errwidth=1,
            capsize=0.2,
            # height=height,
            # aspect=nginx_width/height,
        )
        annotate_bar_values_k_ax(ax, fontsize, rotation=90)
        sns.despine(ax=ax)
        apply_hatch_ax(ax, patch_legend=False, hatch_list=hatches)
        format(ax.yaxis, "krps")
        g.set(xticks=[], xticklabels=[], xlabel="(a) Nginx")
        g.set_title("Higher is better ↑", fontsize=9, color="navy", weight="bold",
                    x = 0.40, y=1.0, pad=title_pad)
        change_width(g, 0.8)
        return g

    def plot_sqlite(df, ax, what="sqlite", fontsize=7):
        df = df.melt(id_vars=["Unnamed: 0"], var_name="system", value_name="sqlite-seconds")
        df = parse_app_system(df)
        # df = df[df["rootfs"] == "initrd"][df["app"] == what]
        # sqlite does not have experiments with ls/perf/bpf
        names = [ config_name.format(app=what) for config_name in config_names[:3] ]
        df = pd.concat([ df[df["system"] == n] for n in names ])
        df = sort(df, names)
        print(df)

        g = sns.barplot(
            ax=ax,
            data=apply_aliases(df),
            x=column_alias("system"),
            y=column_alias("sqlite-seconds"),
            ci="sd",  # show standard deviation! otherwise with_stddev_to_long_form does not work.
            palette=app_palette,
            edgecolor="k",
            errcolor="black",
            errwidth=1,
            capsize=0.2,
        )

        annotate_bar_values_s2_ax(g, fontsize, rotation=90)
        sns.despine(ax=ax)
        g.set(xticks=[], xticklabels=[], xlabel="(c) SQLite\n60k insertion")
        g.set_title("Lower is better ↓", fontsize=9, color="navy", weight="bold",
                    x = 0.45, y=1, pad=title_pad)
        apply_hatch_ax(ax, patch_legend=True, hatch_list=hatches)
        change_width(g, 0.8)

    def plot_redis(df, ax, what="redis", fontsize=7):
        df = df.melt(id_vars=["Unnamed: 0"], var_name="system", value_name="redis-requests")
        df = parse_app_system(df)
        names = [ config_name.format(app=what) for config_name in config_names ]
        df = pd.concat([ df[df["system"] == n] for n in names ])
        df = sort(df, names)


        g = sns.barplot(
            ax=ax,
            data=apply_aliases(df),
            x=column_alias("direction"),
            y=column_alias("redis-requests"),
            hue=column_alias("system"),
            ci="sd",  # show standard deviation! otherwise with_stddev_to_long_form does not work.
            palette=app_palette,
            edgecolor="k",
            errcolor="black",
            errwidth=1.0,
            # capsize=0.2,
            capsize=0.05,
        )
        annotate_bar_values_M2_ax(g, fontsize=fontsize, rotation=90)
        sns.despine(ax=ax)
        # redis_hatches = ["", "", "..", "..", "**", "**", "..", "..", "**", "**", "..", "..", "**", "**", "..", "..", "**", "**"]
        redis_hatches = []
        for hatch in hatches:
            redis_hatches.append(hatch)
            redis_hatches.append(hatch)
        for idx, bar in enumerate(g.patches):
            bar.set_hatch(redis_hatches[idx%len(redis_hatches)])

        bar_width = 0.07
        change_width(g, bar_width)
        # reduce space between "get" and "set"
        # for idx, bar in enumerate(g.patches):
        #     if idx % 2 != 0:
        #         bar.set_x(bar.get_x() - 0.1)
        g.margins(x=0.001)
        g.set_xlabel("(b) Redis")
        g.tick_params(axis='x', length=0) # remove ticks
        g.set_title("Higher is better ↑", fontsize=9, color="navy", weight="bold", pad=title_pad)
        format(ax.yaxis, "mrps")
        ax.legend([], [], frameon=False) # remove legend
        # g.legend(frameon=False) # (re-)draw legend with hatches
        # sns.move_legend(g, "center left", bbox_to_anchor=(1.00, 0.5))

    fs = 9 # font size of the valeus top of the bars
    plot_nginx(df, axs[0], fontsize=fs)
    plot_redis(df, axs[1], fontsize=fs)
    plot_sqlite(df, axs[2], fontsize=fs)

    # legend
    handles = []
    handles.append(mpl.patches.Patch(facecolor=app_palette[0], hatch=hatches[0], edgecolor="k", label='Unikraft'))
    # put a dummy legend to adjust position
    handles.append(mpl.patches.Patch(facecolor="white", hatch="", edgecolor="white", label=f''))
    handles.append(mpl.patches.Patch(facecolor=app_palette[1], hatch=hatches[1], edgecolor="k", label=f'{sysname}*'))
    handles.append(mpl.patches.Patch(facecolor=app_palette[2], hatch=hatches[2], edgecolor="k", label=f'{sysname}'))
    handles.append(mpl.patches.Patch(facecolor=app_palette[3], hatch=hatches[3], edgecolor="k", label=f'{sysname}* w/ ls'))
    handles.append(mpl.patches.Patch(facecolor=app_palette[4], hatch=hatches[4], edgecolor="k", label=f'{sysname}  w/ ls'))
    # handles.append(mpl.patches.Patch(facecolor=app_palette[5], hatch=hatches[5], edgecolor="k", label=f'{sysname}* w/ perf'))
    # handles.append(mpl.patches.Patch(facecolor=app_palette[6], hatch=hatches[6], edgecolor="k", label=f'{sysname}  w/ perf'))
    handles.append(mpl.patches.Patch(facecolor=app_palette[5], hatch="--..", edgecolor="k", label=f'{sysname}* w/ perf'))
    handles.append(mpl.patches.Patch(facecolor=app_palette[6], hatch="--**", edgecolor="k", label=f'{sysname}  w/ perf'))
    handles.append(mpl.patches.Patch(facecolor=app_palette[7], hatch=hatches[7], edgecolor="k", label=f'{sysname}* w/ bpf'))
    handles.append(mpl.patches.Patch(facecolor=app_palette[8], hatch=hatches[8], edgecolor="k", label=f'{sysname}  w/ bpf'))
    fig.legend(handles=handles, loc="lower center", bbox_to_anchor=(0.5, -0.25), frameon=False, ncol=5)

    fig.tight_layout()

    return fig


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


def main_old() -> None:
    """
    Create graph of
        1. unikraft (baseline)
        2. ushell (nompk)
        3. isolated-ushell (with mpk)
    """

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
            graphs.append(("run", console(df, "ushell_run", aspect = 2.3, names=["ushell_run", "ushellmpk_run", "ushell-run-cached", "ushellmpk-run-cached"])))
            graphs.append(("app", app(df)))
        elif name.startswith("console"):
            graphs.append(("console", console(df, "ushell-console", aspect = 1.8, names=["qemu_ssh_console", "ushell-console", "ushellmpk-console", "ushell-console-nginx", "ushellmpk-console-nginx"])))
            graphs.append(("init", console(df, "ushell-init")))
        elif name.startswith("image"):
            graphs.append(("images", images(df, "image-sizes")))
        else:
            print(f"unhandled graph name: {tsv_path}", file=sys.stderr)
            sys.exit(1)

    for prefix, graph in graphs:
        fname = f"{prefix}{out_format}"
        print(f"write {fname}")
        graph.savefig(MEASURE_RESULTS / fname, bbox_inches='tight')


def main() -> None:
    """
    Create graph of
        1. unikraft (baseline)
        3. ushell* (w/o mpk, w/ bpf tracing (mcount))
        3. ushell (w/ mpk, w/ bpf tracing (mcount))
    For file size measurement, nops for mcount are not added
    """

    if len(sys.argv) < 2:
        print(f"USAGE: {sys.argv[0]} graph.tsv...")

    graphs = []
    # aspect = 2.3
    aspect = 2.0

    for arg in sys.argv[1:]:
        tsv_path = Path(arg)
        df = pd.read_csv(tsv_path, sep="\t")
        assert isinstance(df, pd.DataFrame)
        name = tsv_path.stem

        if name.startswith("console"):
            responsiveness = console(df, "ushell-console", aspect=aspect,
                                     names=[ #"qemu_ssh_console",
                                            "ushell-bpf-console",
                                            "ushell-bpf-console-nginx",
                                            "ushellmpk-bpf-console",
                                            "ushellmpk-bpf-console-nginx",
                                            ])
            graphs.append(("console", responsiveness))
        elif name.startswith("memory"):
            names = ["uk-count-noshell", "uk-count-ushellmpk-bpf",
                     "uk-redis-noshell-initrd", "uk-redis-ushellmpk-bpf-initrd",
                     "uk-nginx-noshell-initrd", "uk-nginx-ushellmpk-bpf-initrd",
                     "uk-sqlite3_backup-noshell-initrd", "uk-sqlite3_backup-ushellmpk-bpf-initrd",
                    ]
            names2 = ["uk-count-ushellmpk-bpf",
                      "uk-redis-ushellmpk-bpf-initrd",
                      "uk-nginx-ushellmpk-bpf-initrd",
                      "uk-sqlite3_backup-ushellmpk-bpf-initrd",
                    ]
            baseline = ["uk-count-noshell",
                        "uk-redis-noshell-initrd",
                        "uk-nginx-noshell-initrd",
                        "uk-sqlite3_backup-noshell-initrd",
                    ]
            graphs.append(("memory-unikernel", memory(df, names, value_name="max_mem_use")))
            graphs.append(("memory-host", memory(df, names, value_name="total_host_mem_with_shell_peak")))
            graphs.append(("memory-unikernel-rel", memory2(df, names2, baseline, value_name="max_mem_use")))
            graphs.append(("memory-host-rel", memory2(df, names2, baseline, value_name="total_host_mem_with_shell_peak")))
            graphs.append(("memory-rel", memory3(df, names2, baseline, aspect=aspect)))
        elif name.startswith("app"):
            has_mcount_eval = False
            for column in df.columns:
                if column.find("nomcount") != -1:
                    has_mcount_eval = True
                    break

            graphs.append(("load", console(df, "ushell_run", aspect=aspect,
                                          names=[
                                                  "ushell-bpf_run",
                                                  "ushell-bpf-run-cached",
                                                  "ushellmpk-bpf_run",
                                                  "ushellmpk-bpf-run-cached"
                                                 ]
                                          )))
            # w/ symbol load time
            graphs.append(("load2", console(df, "ushell_run", aspect=aspect,
                                          names=[
                                                  "ushell-bpf_load_sym",
                                                  "ushellmpk-bpf_load_sym",
                                                  "ushell-bpf_run",
                                                  "ushellmpk-bpf_run",
                                                  "ushell-bpf-run-cached",
                                                  "ushellmpk-bpf-run-cached"
                                                 ]
                                          )))
            if has_mcount_eval:
                graphs.append(("app-nomcount", app2(df, [
                                                "{app}_noshell_initrd_nohuman",
                                                "{app}_ushell_bpf-nomcount_initrd_nohuman",
                                                "{app}_ushellmpk_bpf-nomcount_initrd_nohuman"
                                               ],
                                           )))
                graphs.append(("app-mcount", app2(df, [
                                                "{app}_noshell-mcount_initrd_nohuman",
                                                "{app}_ushell_bpf_initrd_nohuman",
                                                "{app}_ushellmpk_bpf_initrd_nohuman"
                                               ],
                                           )))
                graphs.append(("app-nobpf", app2(df, [
                                            "{app}_noshell_initrd_nohuman",
                                            "{app}_ushell_initrd_nohuman",
                                            "{app}_ushellmpk_initrd_nohuman"
                                           ],
                                           )))
            graphs.append(("app", app2(df, [
                                            "{app}_noshell_initrd_nohuman",
                                            "{app}_ushell_bpf_initrd_nohuman",
                                            "{app}_ushellmpk_bpf_initrd_nohuman"
                                           ],
                                       )))
            graphs.append(("app2", app3(df, [
                                             "{app}_noshell_initrd_nohuman",
                                             "{app}_ushell_bpf_initrd_nohuman",
                                             "{app}_ushellmpk_bpf_initrd_nohuman",
                                             "{app}_ushellmpk_bpf_initrd_lshuman",
                                             "{app}_ushellmpk_bpf_initrd_perf",
                                           ],
                                       )))
            graphs.append(("app2-all", app4(df, [
                                             "{app}_noshell_initrd_nohuman",
                                             "{app}_ushell_bpf_initrd_nohuman",
                                             "{app}_ushellmpk_bpf_initrd_nohuman",
                                             "{app}_ushell_bpf_initrd_lshuman",
                                             "{app}_ushellmpk_bpf_initrd_lshuman",
                                             "{app}_ushell_bpf_initrd_perf",
                                             "{app}_ushellmpk_bpf_initrd_perf",
                                             "{app}_ushell_bpf_initrd_bpf",
                                             "{app}_ushellmpk_bpf_initrd_bpf",
                                           ], aspect=3.7,
                                       )))
        elif name.startswith("image"):
            graphs.append(("images-lto", images2(df,
                                             [
                                                 "uk-count-noshell-lto",
                                                 "uk-nginx-noshell-initrd-lto",
                                                 "uk-redis-noshell-initrd-lto",
                                                 # "uk-sqlite_benchmark-noshell-initrd-lto",
                                                 "uk-sqlite3_backup-noshell-initrd-lto",
                                                 "uk-count-ushellmpk-bpf-nomcount-lto",
                                                 "uk-nginx-ushellmpk-bpf-nomcount-initrd-lto",
                                                 "uk-redis-ushellmpk-bpf-nomcount-initrd-lto",
                                                 "uk-sqlite3_backup-ushellmpk-bpf-nomcount-initrd-lto",
                                             ], aspect=aspect,
                                             )))
            graphs.append(("images-nobpf", images2(df,
                                             [
                                                 "uk-count-noshell-lto",
                                                 "uk-nginx-noshell-initrd-lto",
                                                 "uk-redis-noshell-initrd-lto",
                                                 # "uk-sqlite_benchmark-noshell-initrd-lto",
                                                 "uk-sqlite3_backup-noshell-initrd-lto",
                                                 "uk-count-ushellmpk-lto",
                                                 "uk-nginx-ushellmpk-initrd-lto",
                                                 "uk-redis-ushellmpk-initrd-lto",
                                                 "uk-sqlite3_backup-ushellmpk-initrd-lto",
                                             ], aspect=aspect,
                                             )))
        else:
            print(f"unhandled graph name: {tsv_path}", file=sys.stderr)
            continue

    for prefix, graph in graphs:
        fname = f"{prefix}{out_format}"
        print(f"write {fname}")
        graph.savefig(MEASURE_RESULTS / fname, bbox_inches='tight')

if __name__ == "__main__":
    # main_old()
    main()
