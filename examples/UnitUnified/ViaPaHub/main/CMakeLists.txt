# Dependencies (M5Unit-ENV, M5Unit-HUB, M5UnitUnified, M5Unified, ...) are pulled in via main/idf_component.yml
# and auto-added as requirements by the component manager.
idf_component_register(
    SRCS "ViaPaHub.cpp"
    INCLUDE_DIRS ".")
