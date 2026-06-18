"""Shared helpers for PCG validation in editor."""
from __future__ import annotations

import time

try:
    import unreal
except ImportError:
    unreal = None  # type: ignore


def pump_editor(_delta: float = 0.016) -> None:
    if unreal is None:
        return
    try:
        unreal.SystemLibrary.execute_console_command(None, "obj gc", None)
    except Exception:
        pass


def is_generating(comp) -> bool:
    if hasattr(comp, "is_generating"):
        try:
            return bool(comp.is_generating())
        except Exception:
            pass
    return False


def generate_and_wait(comp, force: bool = True, max_wait: float = 30.0) -> None:
    comp.set_editor_property("b_activated", True)
    lib = getattr(unreal, "MelodiaPCGLibrary", None)
    if lib and hasattr(lib, "generate_pcg_component"):
        try:
            lib.generate_pcg_component(comp, force, max_wait)
            return
        except TypeError:
            try:
                lib.generate_pcg_component(comp, force)
                return
            except TypeError:
                pass
    comp.generate(force=force)
    deadline = time.time() + max_wait
    idle = 0
    while time.time() < deadline:
        if not is_generating(comp):
            idle += 1
            if idle >= 5:
                break
        else:
            idle = 0
        pump_editor()
        time.sleep(0.05)


def count_ism(actor) -> int:
    lib = getattr(unreal, "MelodiaPCGLibrary", None)
    if lib and hasattr(lib, "count_instanced_mesh_instances"):
        try:
            return int(lib.count_instanced_mesh_instances(actor))
        except Exception:
            pass
    return sum(
        c.get_instance_count()
        for c in actor.get_components_by_class(unreal.InstancedStaticMeshComponent)
    )
