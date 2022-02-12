"""
Find and print all todo notes for trivially sized source files.

TODO note structure examples in c++:

// TODO: This is the title.
// This is the body

// TODO: This is the title. This is the body.
// This is also the body


"""
import re
import os
import glob
from collections import namedtuple
from operator import itemgetter, attrgetter

TodoNote = namedtuple("TodoNote", ["filename", "line", "title", "body"])

reLineComment = re.compile(r"\s*//.*\n")
reTodoStart = re.compile(r"\s*//.*\n")
reTodoNote = re.compile(r"\s*//\s?TODO:\s?((?:.*\n)(?:\s*//.*\n)*)")

# script is intended to be accessed from top project directory
project_dir = os.getcwd()
cwd = project_dir + "\\src\\"
output = project_dir + "\\tools\\todo-notes-output.txt"

files = []
for ext in ("*.cpp", "*.hpp", "*.h", ".c"):
    files.extend(glob.glob(os.path.join(cwd, ext)))


def ParseMatch(text):
    lines = text.split("\n")
    # Find title by either 1st whole line or first sentance
    # see file comment
    if match := re.match(r"(.*\.)(.*)", lines[0]):
        title = match.group(1)
        body = match.group(2).strip(" ")
    else:
        title = lines[0]
        body = ""

    for line in lines[1:]:
        body += "\n" + line.strip("\n /")
    return title, body.strip("\n")


notes = []
for path in files:
    with open(path, "rt") as f:
        basename = os.path.basename(path)
        text = f.read()
        if matches := reTodoNote.finditer(text):
            for match in matches:
                title, body = ParseMatch(match.group(1))
                line_number = text[: match.start(1)].count("\n") + 1
                notes.append(TodoNote(basename, line_number, title, body))

notes = sorted(notes, key=lambda x: attrgetter("filename")(x).lower())

with open(output, "wt") as f:
    header = f"Todo items remaining: {len(notes)}\n"
    header += ("-" * 60) + "\n"
    f.write(header)
    for note in notes:
        msg = f"""Source: {note.filename}
Line #: {note.line}
Title: {note.title}
Body: {note.body}\n"""
        msg += "\n\n"
        f.write(msg)

os.startfile(output)
