/*
 * Copyright 2013  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://floralicense.org
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <Elementary.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <libgen.h>

#include <Ecore.h>

#include <appcore-efl.h>
#include <dlog.h>
#include <bundle.h>

#include "debug.h"
#include "CController.h"

static int app_create(void *data)
{
	return 0;
}

static int app_terminate(void *data)
{
	return CController::Finalize();
}

static int app_pause(void *data)
{
	return 0;
}

static int app_resume(void *data)
{
	return 0;
}

static int app_reset(bundle *b, void *data)
{
	const char *name;

	name = bundle_get_val(b, "name");
	if (!name) {
		ErrPrint("Name is not valid\n");
		elm_exit();
		return -EINVAL;
	}

	return CController::Initialize(name);
}

int main(int argc, char *argv[])
{
	struct appcore_ops ops;

	ops.create = app_create;
	ops.terminate = app_terminate;
	ops.pause = app_pause;
	ops.resume = app_resume;
	ops.reset = app_reset;
	ops.data = NULL;

	return appcore_efl_main(basename(argv[0]), &argc, &argv, &ops);
}

/* End of a file */
