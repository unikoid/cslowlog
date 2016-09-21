FROM ubuntu
RUN apt-get update && apt-get install -y bash build-essential
CMD bash
