

class GeneratedNamespace(object):
    def __init__(self, parent: object, namespace: str):
        self.parent = parent

        self.ns = namespace
        self.lines = []

    def __enter__(self):
        self.lines.append(f"namespace {self.ns} {{")

        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.lines.append("}")

        self.parent.append_lines(self.lines)

    def append_lines(self, lines: list[str]):
        self.lines.extend(map(lambda x : "    " + x, lines))


