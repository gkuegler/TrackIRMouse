/**
 * Main app entry and GUI components.
 *
 * --License Boilerplate Placeholder--
 *
 */
#pragma once
#ifndef TIRMOUSE_SETTINGS_LOAD_HPP
#define TIRMOUSE_SETTINGS_LOAD_HPP

/**
 * Returns true if application should continue loading.
 * Returns false if application should exit.
 * Guard loading of configuration from a file with the user dialog, if operation
 * fails.
 */
bool
LoadSettingsFile();

#endif // TIRMOUSE_SETTINGS_LOAD_HPP