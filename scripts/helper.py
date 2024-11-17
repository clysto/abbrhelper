import csv
from rapidfuzz import fuzz
import queue
from dataclasses import dataclass
import sys

JOURNAL_DATABASE = []


def init():
    global JOURNAL_DATABASE
    JOURNAL_DATABASE = list(csv.DictReader(open("journals.csv")))


@dataclass
class SearchResult:
    def __init__(self, score, journal):
        self.score = score
        self.journal = journal

    def __lt__(self, other):
        return self.score < other.score

    def __eq__(self, other):
        return self.score == other.score

    def __gt__(self, other):
        return self.score > other.score


def search_journal(name):
    matched = queue.PriorityQueue(maxsize=9)
    for j in JOURNAL_DATABASE:
        name = name.lower()
        score = max(
            fuzz.ratio(name, j["Title"].lower()),
            fuzz.ratio(name, j["JCR Abbr"].lower()),
            fuzz.ratio(name, j["ISO Abbr"].lower()),
            fuzz.ratio(name, j["Med Abbr"].lower()),
        )
        matched.put(SearchResult(score, j))
        if matched.full():
            matched.get()
    return matched


if __name__ == "__main__":
    input_file = sys.argv[1]
    output_file = sys.argv[2]

    init()
    with open(input_file) as f:
        lines = f.readlines()
        lines = [line.strip() for line in lines]
        names = sorted(list(set(lines)))
        abbrs = []
        for name in names:
            print(f"Searching for {name}...")
            matched = search_journal(name)
            matched_list = []
            while matched.empty() is False:
                result = matched.get()
                matched_list.append(result)
            matched_list = matched_list[::-1]
            print(f"Choose the correct journal abbreviation for {name}:")
            for index, result in enumerate(matched_list):
                print(
                    f"{index+1}. {result.journal['ISO Abbr']} ({result.journal['Title']})"
                )
            print("0. None of the above")

            choice = int(input("Enter the number of the correct abbreviation: "))

            if choice == 0:
                abbr = input("Enter the correct abbreviation: ")
                abbrs.append(abbr)
            else:
                abbrs.append(matched_list[choice - 1].journal["ISO Abbr"])
            print("\033[H\033[J")

    print("Results:")
    for name, abbr in zip(names, abbrs):
        print(f"{name} -> {abbr}")

    with open(output_file, "w") as f:
        for name, abbr in zip(names, abbrs):
            f.write(f"{name}\t{abbr}\n")

    print(f"Endnote Term List file has been saved to {output_file}")
