/*******************************************************
 * Copyright (c) 2014, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#include <gtest/gtest.h>
#include <arrayfire.h>
#include <af/dim4.hpp>
#include <af/traits.hpp>
#include <vector>
#include <iostream>
#include <string>
#include <testHelpers.hpp>

using std::vector;
using std::string;
using std::cout;
using std::endl;

const size_t step_bytes = 1024;

void cleanSlate()
{
    size_t alloc_bytes, alloc_buffers;
    size_t lock_bytes, lock_buffers;

    af::deviceGC();

    af::deviceMemInfo(&alloc_bytes, &alloc_buffers,
                      &lock_bytes, &lock_buffers);

    ASSERT_EQ(alloc_buffers, 0u);
    ASSERT_EQ(lock_buffers, 0u);
    ASSERT_EQ(alloc_bytes, 0u);
    ASSERT_EQ(lock_bytes, 0u);

    af::setMemStepSize(step_bytes);

    ASSERT_EQ(af::getMemStepSize(), step_bytes);
}

TEST(Memory, GetDevicePtr)
{
    size_t alloc_bytes, alloc_buffers;
    size_t lock_bytes, lock_buffers;

    cleanSlate(); // Clean up everything done so far

    af::array a = af::randu(5, 5);

    af::deviceMemInfo(&alloc_bytes, &alloc_buffers,
                      &lock_bytes, &lock_buffers);

    ASSERT_EQ(alloc_buffers, 1u);
    ASSERT_EQ(lock_buffers, 1u);
    ASSERT_EQ(alloc_bytes, 1 * step_bytes);
    ASSERT_EQ(lock_bytes, 1 * step_bytes);

    a.device<float>();

    af::deviceMemInfo(&alloc_bytes, &alloc_buffers,
                      &lock_bytes, &lock_buffers);

    ASSERT_EQ(alloc_buffers, 1u);
    ASSERT_EQ(lock_buffers, 0u); // 0 because device should unlock the buffer
    ASSERT_EQ(alloc_bytes, 1 * step_bytes);
    ASSERT_EQ(lock_bytes, 0u);
}

TEST(Memory, Scope)
{
    size_t alloc_bytes, alloc_buffers;
    size_t lock_bytes, lock_buffers;

    cleanSlate(); // Clean up everything done so far

    {
        af::array a = af::randu(5, 5);

        af::deviceMemInfo(&alloc_bytes, &alloc_buffers,
                          &lock_bytes, &lock_buffers);

        ASSERT_EQ(alloc_buffers, 1u);
        ASSERT_EQ(lock_buffers, 1u);

        ASSERT_EQ(alloc_bytes, 1 * step_bytes);
        ASSERT_EQ(lock_bytes, 1 * step_bytes);
    }


    af::deviceMemInfo(&alloc_bytes, &alloc_buffers,
                      &lock_bytes, &lock_buffers);

    ASSERT_EQ(alloc_buffers, 1u);
    ASSERT_EQ(lock_buffers, 0u); // 0 because a is out of scope

    ASSERT_EQ(alloc_bytes, 1 * step_bytes);
    ASSERT_EQ(lock_bytes, 0u);
}

TEST(Memory, SingleSizeLoop)
{
    size_t alloc_bytes, alloc_buffers;
    size_t lock_bytes, lock_buffers;

    cleanSlate(); // Clean up everything done so far

    {
        af::array a = af::randu(5, 5);

        af::deviceMemInfo(&alloc_bytes, &alloc_buffers,
                      &lock_bytes, &lock_buffers);

        ASSERT_EQ(alloc_buffers, 1u);
        ASSERT_EQ(lock_buffers, 1u);
        ASSERT_EQ(alloc_bytes, 1 * step_bytes);
        ASSERT_EQ(lock_bytes, 1 * step_bytes);

        for (int i = 0; i < 100; i++) {

            a = af::randu(5,5);

            af::deviceMemInfo(&alloc_bytes, &alloc_buffers,
                              &lock_bytes, &lock_buffers);

            ASSERT_EQ(alloc_buffers, 2u); //2 because a new one is created before a is destroyed
            ASSERT_EQ(lock_buffers, 1u);
            ASSERT_EQ(alloc_bytes, 2 * step_bytes);
            ASSERT_EQ(lock_bytes, 1 * step_bytes);
        }
    }
}

TEST(Memory, LargeLoop)
{
    size_t alloc_bytes, alloc_buffers;
    size_t lock_bytes, lock_buffers;

    cleanSlate(); // Clean up everything done so far

    const int num = step_bytes / sizeof(float);
    size_t allocated = step_bytes;

    af::array a = af::randu(num);

    std::vector<float> hA(num);

    a.host(&hA[0]);

    // Run a large loop that allocates more and more memory at each step
    for (int i = 0; i < 250; i++) {
        af::array b = af::randu(num * (i + 1));
        size_t current = (i + 1) * step_bytes;
        allocated += current;

        // Verify that new buffers are being allocated
        af::deviceMemInfo(&alloc_bytes, &alloc_buffers,
                          &lock_bytes, &lock_buffers);

        // Limit to 10 to check before garbage collection
        if (i < 10) {
            ASSERT_EQ(alloc_buffers, (size_t)(i + 2)); //i is zero based
            ASSERT_EQ(lock_buffers, 2u);

            ASSERT_EQ(alloc_bytes, allocated);
            ASSERT_EQ(lock_bytes, current + step_bytes);
        }
    }

    size_t old_alloc_bytes = alloc_bytes;
    size_t old_alloc_buffers = alloc_buffers;

    af::deviceMemInfo(&alloc_bytes, &alloc_buffers,
                      &lock_bytes, &lock_buffers);

    ASSERT_EQ(old_alloc_bytes, alloc_bytes);
    ASSERT_EQ(old_alloc_buffers, alloc_buffers);

    ASSERT_EQ(lock_buffers, 1u);
    ASSERT_EQ(lock_bytes, 1 * step_bytes);
}
