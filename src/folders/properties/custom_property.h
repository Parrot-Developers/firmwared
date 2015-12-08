/**
 * @file custom_property.h
 * @brief
 *
 * @date 4 déc. 2015
 * @author ncarrier
 * @copyright Copyright (C) 2015 Parrot S.A.
 */

#ifndef SRC_FOLDERS_PROPERTIES_CUSTOM_PROPERTY_H_
#define SRC_FOLDERS_PROPERTIES_CUSTOM_PROPERTY_H_
#include <stdbool.h>

#include "folders.h"

bool is_custom_property(const struct folder_property *property);
struct folder_property *custom_property_new(const char *name);
/* removes all the custom property values stored for the given entity */
void custom_property_cleanup_values(const struct folder_entity *entity);
void custom_property_delete(struct folder_property **property);

#endif /* SRC_FOLDERS_PROPERTIES_CUSTOM_PROPERTY_H_ */
