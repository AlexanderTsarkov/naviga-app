#!/usr/bin/env python3
"""
Guardrail: prevent Arduino/ESP dependencies leaking into core/app/services.
"""
from __future__ import annotations

import os
import sys
from dataclasses import dataclass
from typing import Iterable


@dataclass(frozen=True)
class ScanRule:
    name: str
    root: str
    tokens: tuple[str, ...]


EXTENSIONS = (".h", ".hpp", ".c", ".cpp", ".ino")


def iter_files(root: str) -> Iterable[str]:
    for dirpath, _, filenames in os.walk(root):
        for filename in filenames:
            if filename.endswith(EXTENSIONS):
                yield os.path.join(dirpath, filename)


def scan_file(path: str, tokens: Iterable[str]) -> list[str]:
    try:
        with open(path, "r", encoding="utf-8", errors="ignore") as handle:
            content = handle.read()
    except OSError:
        return []
    found = [token for token in tokens if token in content]
    return found


def scan_rule(rule: ScanRule) -> list[str]:
    issues: list[str] = []
    for path in iter_files(rule.root):
        found = scan_file(path, rule.tokens)
        for token in found:
            rel_path = os.path.relpath(path)
            issues.append(f"{rule.name}: {rel_path} -> {token}")
    return issues


def main() -> int:
    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    core_root = os.path.join(repo_root, "firmware", "lib", "NavigaCore")
    app_root = os.path.join(repo_root, "firmware", "src", "app")
    services_root = os.path.join(repo_root, "firmware", "src", "services")

    core_tokens = ("Arduino.h", "Serial", "millis(", "delay(", "esp_", "ESP_OK", "esp_efuse")
    app_tokens = ("Arduino.h", "Serial", "delay(", "millis(", "esp_", "ESP_OK", "esp_efuse")

    rules = (
        ScanRule("core", core_root, core_tokens),
        ScanRule("app", app_root, app_tokens),
        ScanRule("services", services_root, app_tokens),
    )

    issues: list[str] = []
    for rule in rules:
        issues.extend(scan_rule(rule))

    if issues:
        print("Platform leak check failed:", file=sys.stderr)
        for issue in issues:
            print(f"  - {issue}", file=sys.stderr)
        return 1

    print("Platform leak check passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
