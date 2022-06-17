// This file is part of Geoflow
// Copyright (C) 2018-2022 Ravi Peters

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
#include "geoflow.hpp"
#ifdef GF_BUILD_WITH_GUI
  #include "imgui.h"
  #include "gui/parameter_widgets.hpp"
#endif

#include <chrono>
#include <ctime>
// #include <taskflow/taskflow.hpp>

namespace geoflow::nodes::core {

  class ProxyNode : public Node {
    public:
    using Node::Node;
    void init(){};
    void process(){};
  };

  class IntNode : public Node {
    int value_=0;
    public:
    using Node::Node;
    void init(){
      add_output("value", typeid(int));
      add_param(ParamInt(value_, "value", "Integer value"));
    };
    void process(){
      output("value").set(value_);
    };
  };
  class FloatNode : public Node {
    float value_=0;
    public:
    using Node::Node;
    void init(){
      add_output("value", typeid(float));
      add_param(ParamFloat(value_, "value", "Floating point value"));
    };
    void process(){
      output("value").set(value_);
    };
  };
  class BoolNode : public Node {
    bool value_=true;
    public:
    using Node::Node;
    void init(){
      add_output("value", typeid(bool));
      add_param(ParamBool(value_, "value", "Boolean value"));
    };
    void process(){
      output("value").set(value_);
    };
  };
  class TextNode : public Node {
    std::string value_="";
    public:
    using Node::Node;
    void init(){
      add_output("value", typeid(std::string));
      add_param(ParamText(value_, "value", "Text value"));
    };
    void process(){
      output("value").set( manager.substitute_globals(value_) );
    };
  };


  class TextWriterNode : public Node {
    std::string filepath_="";
    public:
    using Node::Node;
    void init(){
      add_input("value", typeid(std::string));
      add_param(ParamPath(filepath_, "filepath", "File path"));
    };
    void process(){
      auto value = input("value").get<std::string>();

      auto fname = manager.substitute_globals(filepath_);
      
      fs::create_directories(fs::path(fname).parent_path());
      
      std::ofstream ofs;
      ofs.open(fname.c_str());
      ofs << value;
      ofs.close();
    };
  };

  class NestNode : public Node {
    private:
    bool flowchart_loaded=false;
    bool use_parallel_processing=false;
    std::string filepath_;
    std::unique_ptr<NodeManager> nested_node_manager_;
    // std::vector<std::weak_ptr<gfInputTerminal>> nested_inputs_;
    // std::vector<std::weak_ptr<gfOutputTerminal>> nested_outputs_;
    std::string proxy_node_name_ = "ProxyNode";
    size_t input_size_=0;

    bool load_nodes() {
      if (fs::exists(filepath_)) {
        input_terminals.clear();
        output_terminals.clear();
        nested_node_manager_->clear();

        add_poly_input(get_name()+".globals", {typeid(int), typeid(float), typeid(bool), typeid(std::string), typeid(Date), typeid(Time), typeid(DateTime)});
        nested_node_manager_->set_globals(get_manager());
        // nested_outputs_.clear();
        // nested_inputs_.clear();
        // load nodes from json file
        auto nodes = nested_node_manager_->load_json(filepath_);
        // find inputs and outputs to connect to this node's terminals...
        // create vectormonoinputs/outputs on this node
        for (auto& node : nodes) {
          auto& node_name = node->get_name();
          for (auto [name, input_term] : node->input_terminals) {
            if (input_term->is_marked()) {
              if(input_term->get_family() == GF_SINGLE_FEATURE)
                add_vector_input(input_term->get_full_name(), input_term->get_types());
              else
                add_poly_input(input_term->get_full_name(), input_term->get_types());
            }
          }
          for (auto [name, output_term_] : node->output_terminals) {
            if (output_term_->is_marked()) {
              if (output_term_->get_family() == GF_SINGLE_FEATURE) {
                auto output_term = (gfSingleFeatureOutputTerminal*)(output_term_.get());
                add_vector_output(output_term->get_full_name(), output_term->get_type());
              } else {
                auto output_term = (gfMultiFeatureOutputTerminal*)(output_term_.get());
                add_poly_output(output_term->get_full_name(), output_term->get_types());
              }
            }
          }
        }
        // output terminal for outputting the execution time for each run inside this nestnode
        add_vector_output(get_name()+".timings", typeid(float));
        return true;
      }
      return false;
    }

    public:
    using Node::Node;

    void init() {
      nested_node_manager_ = std::make_unique<NodeManager>(manager.get_node_registers()); // this will only transfer the node registers
      add_param(ParamPath(filepath_, "filepath", "Flowchart file"));
      // add_param(ParamBool(use_parallel_processing, "use_parallel_processing", "Use parallel processing"));

    };
    bool inputs_valid() {
      for (auto& [name,iT] : input_terminals) {
        if (iT->get_name() == get_name()+".globals")
          continue;
        else if (!iT->has_data())
          return false;
      }
      return true;
    }
    void post_parameter_load() {
      flowchart_loaded = load_nodes();
    }

    #ifdef GF_BUILD_WITH_GUI
      void gui() {
        for( auto& [name, node] : nested_node_manager_->get_nodes() ) {
          ImGui::PushID(node.get());
          if (ImGui::CollapsingHeader(name.c_str())) {
            geoflow::draw_parameters(node);
          }
          ImGui::PopID();
        }
        ImGui::Separator();
        if(ImGui::Button("Load Nodes"))
          flowchart_loaded = load_nodes();
        ImGui::SameLine();
        if(ImGui::Button("Sync globals"))
          for (auto& [key,val] : manager.global_flowchart_params) {
            nested_node_manager_->global_flowchart_params[key] = val;
          }
      };
    #endif

    std::shared_ptr<NodeManager> copy_nested_flowchart() {
      auto flowchart = std::make_shared<NodeManager>(*nested_node_manager_);
      flowchart->data_offset = *manager.data_offset;
      // set up proxy node
      auto R = std::make_shared<NodeRegister>("ProxyRegister");
      R->register_node<ProxyNode>("Proxy");
      // create proxy
      auto proxy_node = flowchart->create_node(R, "Proxy");
      flowchart->name_node(proxy_node, proxy_node_name_);
      // create proxy outputs to nested fc inputs
      for (auto& [node_name, node] : flowchart->get_nodes()) {
          for (auto& [name, input_term] : node->input_terminals) {
            if (input_term->is_marked()) {
              auto input_name = input_term->get_full_name();
              if(input_term->get_family() == GF_SINGLE_FEATURE) {
                proxy_node->add_output(input_name, input_term->get_types());
                proxy_node->output(input_name).connect(*input_term);
              } else { // GF_MULTI_FEATURE
                proxy_node->add_poly_output(input_name, input_term->get_types());
                auto mf_oterm = proxy_node->poly_output(input_name);
                proxy_node->poly_output(input_name).connect(*input_term);
              }
            }
          }
      }

      return flowchart;
    }
    void set_inputs(std::shared_ptr<NodeManager>& flowchart, size_t i) {
      auto proxy_node = flowchart->get_node(proxy_node_name_);
      // note that proxy node has no inputs
      
      for(auto& [name, proxy_output] : proxy_node->output_terminals) {
        if (proxy_output->get_family()==GF_SINGLE_FEATURE) {
          auto& data_vec = vector_input(name).get_data_vec();
          // we need to set the correct type
          proxy_node->output(name).set_type(vector_input(name).get_connected_type());
          proxy_node->output(name).set_from_any(data_vec[i]);
        } else {
          for (auto sub_iterm : poly_input(name).sub_terminals()) {
            auto& sub_name = sub_iterm->get_name();
            // first add sub terminal
            auto& sub_oterm = proxy_node->poly_output(name).add(sub_name, sub_iterm->get_types()[0]);
            sub_oterm.set_from_any(sub_iterm->get_data_vec()[i]);
          }
        }
      }
    }

    void process_parallel() {
      // repack input data
      // assume all vector inputs have the same size

      // std::vector<std::shared_ptr<NodeManager>> flowcharts;
      // for(size_t i=0; i<input_size_; ++i) {
      //   auto fc = copy_nested_flowchart();
      //   for (auto& [key,val] : manager.global_flowchart_params) {
      //     fc->global_flowchart_params[key] = val;
      //   }
      //   fc->global_flowchart_params["GF_I"] = std::make_shared<ParameterByValue<std::string>>(std::to_string(i), "GF_I", "");
      //   set_inputs(fc, i);
      //   flowcharts.push_back(fc);
      // }

      // tf::Executor executor;
      // tf::Taskflow taskflow;
      // for(auto& fc : flowcharts) {
      //   tf::Task task = taskflow.emplace([&](){ 
      //     auto& proxy_node = fc->get_node(proxy_node_name_);
      //     fc->run(proxy_node, false);
      //    });
      // }
      // executor.run(taskflow).wait();
    };

    void process_sequential() {
      // repack input data
      // assume all vector inputs have the same size
      auto flowchart = copy_nested_flowchart();
      auto& proxy_node = flowchart->get_node(proxy_node_name_);
      float runtime;
      for(size_t i=0; i<input_size_; ++i) {
        proxy_node->notify_children();
        // prep inputs
        for (auto& [key,val] : manager.global_flowchart_params) {
          flowchart->global_flowchart_params[key] = val;
        }
        flowchart->global_flowchart_params["GF_I"] = std::make_shared<ParameterByValue<std::string>>(std::to_string(i), "GF_I", "");

        // create globals from inputs on .globals terminal
        auto& glterm = poly_input(get_name()+".globals");
        for(auto& sterm : glterm.sub_terminals()) {
          if(sterm->accepts_type(typeid(std::string))) {
            auto& val = sterm->get<std::string>(i);
            flowchart->global_flowchart_params[sterm->get_name()] = std::make_shared<ParameterByValue<std::string>>(val, sterm->get_name(), "global from polyinput");
          } else if(sterm->accepts_type(typeid(int))) {
            auto val = sterm->get<int>(i);
            flowchart->global_flowchart_params[sterm->get_name()] = std::make_shared<ParameterByValue<int>>(val, sterm->get_name(), "global from polyinput");
          } else if(sterm->accepts_type(typeid(float))) {
            auto val = sterm->get<float>(i);
            flowchart->global_flowchart_params[sterm->get_name()] = std::make_shared<ParameterByValue<float>>(val, sterm->get_name(), "global from polyinput");
          } else if(sterm->accepts_type(typeid(bool))) {
            auto val = sterm->get<bool>(i);
            flowchart->global_flowchart_params[sterm->get_name()] = std::make_shared<ParameterByValue<bool>>(val, sterm->get_name(), "global from polyinput");
          }
        }

        set_inputs(flowchart, i);
        // run
        std::cout << "Processing item " << i+1 << "/" << input_size_ << "\n";
        std::clock_t c_start = std::clock(); // CPU time
        // auto t_start = std::chrono::high_resolution_clock::now(); // Wall time
        flowchart->run_all(false);
        std::clock_t c_end = std::clock(); // CPU time
        // auto t_end = std::chrono::high_resolution_clock::now(); // Wall time
        runtime = 1000.0 * (c_end-c_start) / CLOCKS_PER_SEC;
        std::cout << ".. " << runtime << "ms\n";
        // collect outputs and push directly to vector outputs
        for (auto& [node_name, node] : flowchart->get_nodes()) {
          for (auto& [term_name, output_term_] : node->output_terminals) {
            if (output_term_->is_marked()) {
              if (output_term_->get_family() == GF_SINGLE_FEATURE) {
                auto output_term = (gfSingleFeatureOutputTerminal*)(output_term_.get());
                if (output_term->has_data()) {
                  for (auto& data : output_term->get_data_vec()) {
                    vector_output(node_name+"."+term_name).push_back_any(data);
                  }
                } else {
                  std::cout << "pushing empty any for " << node_name+"."+term_name << "at i=" << i << std::endl;
                  vector_output(node_name+"."+term_name).push_back_any(std::any());
                }
              } else {
                auto output_term = (gfMultiFeatureOutputTerminal*)(output_term_.get());
                auto& aggregate_poly_out = poly_output(node_name+"."+term_name);
                for (auto& [name, sub_term]: output_term->sub_terminals()) {
                  if(i==0) {
                    aggregate_poly_out.add_vector(name, sub_term->get_type());
                  }
                  for (auto& data : sub_term->get_data_vec()) {
                    aggregate_poly_out.sub_terminal(name).push_back_any(data);
                  }
                }
              }
            }
          }
        }
        vector_output(get_name()+".timings").push_back(runtime);
      }
    };

    void process() {
      if(flowchart_loaded) {
        auto first_input = input_terminals.begin()->second.get();
        input_size_ = first_input->size();
        std::cout << "Begin processing for NestNode " << get_name() << "\n";
        if (use_parallel_processing) {
          process_parallel();
        } else {
          process_sequential();
        }
        std::cout << "End processing for NestNode " << get_name() << "\n";
      }
    }
  };
}