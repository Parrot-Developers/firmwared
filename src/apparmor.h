/**
 * @file apparmor.h
 * @brief
 *
 * @date May 22, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#ifndef APPARMOR_H_
#define APPARMOR_H_

int apparmor_init(void);
int apparmor_load_profile(const char *root, const char *name);
int apparmor_change_profile(const char *name);
void apparmor_cleanup(void);

#endif /* APPARMOR_H_ */
