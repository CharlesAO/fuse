/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2018, Locus Robotics
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the copyright holder nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef FUSE_CORE_CONSTRAINT_H
#define FUSE_CORE_CONSTRAINT_H

#include <fuse_core/uuid.h>
#include <fuse_core/macros.h>

#include <boost/core/demangle.hpp>
#include <ceres/cost_function.h>
#include <ceres/loss_function.h>

#include <initializer_list>
#include <ostream>
#include <string>
#include <vector>


namespace fuse_core
{

/**
 * @brief The Constraint interface definition.
 *
 * A Constraint defines a cost function that is connected to one or more variables. This base class defines the
 * required interface of all Constraint objects, and holds the ordered list of involved variable UUIDs. All other
 * functionality is left to the derived classes to implement.
 *
 * Most importantly, the implementation of the cost function is left to the derived classes, allowing arbitrarily
 * complex sensor models to be implemented outside of the core fuse packages. The cost function must be a valid
 * ceres::CostFunction object. Ceres provides many nice features to make implementing the cost function easier,
 * including an automatic differentiation system. Please see the Ceres documentation for details on creating valid
 * ceres::CostFunction objects (http://ceres-solver.org/nnls_modeling.html). In addition to the cost function itself,
 * an optional loss function may be provided. Loss functions provide a mechanism for reducing the impact of outlier
 * measurements on the final optimization results. Again, see the Ceres documentation for details
 * (http://ceres-solver.org/nnls_modeling.html#lossfunction).
 */
class Constraint
{
public:
  SMART_PTR_ALIASES_ONLY(Constraint);

  /**
   * @brief Constructor
   *
   * Accepts an arbitrary number of variable UUIDs directly. It can be called like:
   * @code{.cpp}
   * Constraint{uuid1, uuid2, uuid3};
   * @endcode
   *
   * @param[in] variable_uuid_list The list of involved variable UUIDs
   */
  Constraint(std::initializer_list<UUID> variable_uuid_list);

  /**
   * @brief Constructor
   *
   * Accepts an arbitrary number of variable UUIDs stored in a container using iterators.
   */
  template<typename VariableUuidIterator>
  Constraint(VariableUuidIterator first, VariableUuidIterator last);

  /**
   * @brief Destructor
   */
  virtual ~Constraint() = default;

  /**
   * @brief Returns a unique name for this constraint type.
   *
   * The constraint type string must be unique for each class. As such, the fully-qualified class name is an excellent
   * choice for the type string.
   */
  virtual std::string type() const { return boost::core::demangle(typeid(*this).name()); }

  /**
   * @brief Returns the UUID for this constraint.
   *
   * Each constraint will generate a unique, random UUID during construction.
   */
  const UUID& uuid() const { return uuid_; }

  /**
   * @brief Print a human-readable description of the constraint to the provided stream.
   *
   * @param  stream The stream to write to. Defaults to stdout.
   */
  virtual void print(std::ostream& stream = std::cout) const = 0;

  /**
   * @brief Create a new Ceres cost function and return a raw pointer to it.
   *
   * The Ceres interface requires a raw pointer. Ceres will take ownership of the pointer and promises to properly
   * delete the cost function when it is done. Additionally, fuse promises that the Constraint object will outlive any
   * generated cost functions (i.e. the Ceres objects will be destroyed before the Constraint objects). This guarantee
   * may allow optimizations for the creation of the cost function objects.
   *
   * @return A base pointer to an instance of a derived ceres::CostFunction.
   */
  virtual ceres::CostFunction* costFunction() const = 0;

  /**
   * @brief Create a new Ceres loss function and return a raw pointer to it.
   *
   * See http://ceres-solver.org/nnls_modeling.html#lossfunction for a detailed description about loss functions.
   * Basically, a loss function defines the penalty associated with a specific amount error. By default, or if a NULL
   * pointer is provided as the loss function, the penalty will be quadratic. This is the loss function associated
   * with standard least-squares optimization. By providing a different loss function, the penalty for the constraint's
   * error can be altered.
   *
   * This is generally done to reduce the effect of outlier measurements that made it into the optimization problem.
   * It is always better to remove outliers before they make it into the optimization problem, but no method is perfect.
   * Using robust loss functions can significantly improve the results and stability of the solution in the presence
   * of outlier measurements.
   *
   * The Ceres interface requires a raw pointer. Ceres will take ownership of the pointer and promises to properly
   * delete the loss function when it is done. Additionally, Fuse promises that the Constraint object will outlive any
   * generated loss functions (i.e. the Ceres objects will be destroyed before the Constraint objects). This guarantee
   * may allow optimizations for the creation of the loss function objects.
   *
   * @return A base pointer to an instance of a derived ceres::LostFunction.
   */
  virtual ceres::LossFunction* lossFunction() const
  {
    return nullptr;
  }

  /**
   * @brief Perform a deep copy of the Constraint and return a unique pointer to the copy
   *
   * This can/should be implemented as follows in all derived classes:
   * @code{.cpp}
   * return Derived::make_unique(*this);
   * @endcode
   *
   * @return A unique pointer to a new instance of the most-derived Constraint
   */
  virtual Constraint::UniquePtr clone() const = 0;

  /**
   * @brief Read-only access to the ordered list of variable UUIDs involved in this constraint
   */
  const std::vector<UUID>& variables() const { return variables_; }

protected:
  UUID uuid_;  //!< The unique ID associated with this constraint
  std::vector<UUID> variables_;  //!< The ordered set of variables involved with this constraint
};

/**
 * Stream operator implementation used for all derived Constraint classes.
 */
std::ostream& operator <<(std::ostream& stream, const Constraint& constraint);


template<typename VariableUuidIterator>
Constraint::Constraint(VariableUuidIterator first, VariableUuidIterator last) :
  uuid_(uuid::generate())
{
  while (first != last)
  {
    variables_.push_back(*first++);
  }
}

}  // namespace fuse_core

#endif  // FUSE_CORE_CONSTRAINT_H
