# syntax=docker/dockerfile:1.2
FROM ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && \
    apt-get install -y \
            build-essential \
            cmake \
            git \
            vim

WORKDIR "/workspaces"
