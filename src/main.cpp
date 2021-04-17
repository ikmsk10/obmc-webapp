// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <string>
#include <ctype.h>
#include <fstream>

#include <core/application.hpp>
#include <config.h>

int main()
{
    app::core::Application app;

	app.configure();
	app.start();

	return EXIT_SUCCESS;
}
