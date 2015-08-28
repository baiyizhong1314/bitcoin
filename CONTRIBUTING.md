# Contributing to Omni Core

## Table of Contents

1. [Reporting Security Issues](#reporting-security-issues)
2. [Reporting Other Issues](#reporting-other-issues)
 1. [Contributors](#contributors)
 2. [Maintainers](#maintainers)
3. [Requesting Features](#requesting-features)
4. [Submitting and Working on Pull Requests](#submitting-and-working-on-pull-requests)
 1. [Contributors](#contributors-1)
 2. [Maintainers](#maintainers-1)
5. [Reviewing Contributions](#reviewing-contributions)
 1. [Contributors](#contributors-2)
 2. [Maintainers](#maintainers-2)
6. [What’s next?](#whats-next)
7. [Glossary](#glossary)

## Reporting Security Issues

Issues with potential security implications should not be published, but
instead communicated via email to security@omnilayer.org.

This includes especially defects and potential exploits, and other
issues that may break consensus of the Omni Layer.

Please don't hesitate, if you are unsure, whether your observation
qualifies as security related issue.

You will receive a confirmation within 48 hours, after a report was received,
and the maintainers of this project will try to keep you in the loop.

All critical security related issues will be disclosed and published, after a
fix was deployed. Please note, if you prefer not to be mentioned as finder of
the issue.

## Reporting Other Issues

##### Contributors

* please try to describe the observed behavior of the issue
* including steps to reproduce an issue is usually extremely helpful
* if relevant, unexpected behavior, such as spikes in CPU load, or
non-responsiveness via RPC may be noted
* please provide a context, if applicable, for example the Omni Core version,
the operating system, whether the issue was observed on mainnet, testnet, ...
* stating the use of Omni Core may be relevant ("exchange operator" vs.
"home user")
* logs, or excerpts of logs, can be helpful, and may be provided
* if logs are very long, it may be preferred to host them via a service such as
[Gist](https://gist.github.com/) or [Pastebin](http://pastebin.com/)

TIP: Sensitive issues, or logs, may also be reported to issues@omnilayer.org.

##### Maintainers

* new issues should be classified and labeled
* usually maintainers should respond to new issues within 24-48 hours
* to keep the contributor in the loop, the progress on an issue, even if there
was no substantial progress, should be communicated frequently
* it should not be assumed that either party, maintainer or contributor, is
familiar with the use case or role of the other
* abandoned issues, which are non-producible and likely not verifiable without
further input, may be closed after one week

## Requesting Features

Proposals, feature requests and new ideas for Omni Core are very welcome!

Changes to the Omni Layer protocol may also be discussed in the issue
section of the protocol specification or on the developer mailing list:

* dev@omnilayer.org
* https://github.com/OmniLayer/spec/issues

## Submitting and Working on Pull Requests

The lifecycle of pull requests:
```
submission --> work in progress --> tests and peer review --> ack  --> merge
                     ^                      |             \
                     |----------------------/              -> nack --> close
                     |                                         |
                     \-----------------------------------------/
```

##### Contributors

* communication is key, and no contributor should ever hesitate to ask for
assistance
* consensus critical changes should be discussed with a broader audience first,
which ideally includes stakeholders (e.g. Mastercoin holders, Quantum Miners
Corp., ...)
* pull requests should be submitted with an [expressive subject line](http://chris.beams.io/posts/git-commit/#seven-rules),
which may be used for change logs
* pull requests should include a description of what the pull request does, or
intends to solve (not necessarily how, which is usually visible due to the code)
* however, complex changes, or changes that are difficult to review, should be
described
* if a pull request [resolves an issue](https://help.github.com/articles/closing-issues-via-commit-messages),
it should be noted in the description
* unrelated changes should be split into more than one submission
* ideally code should be documented in a [doxygen compatible format](http://www.stack.nl/~dimitri/doxygen/manual/docblocks.html#cppblock)
* tests, or a few notes and ideas about how the submission might be tested,
would be awesome, but not mandatory
* ideally commits are [GPG signed](https://git-scm.com/book/tr/v2/Git-Tools-Signing-Your-Work)
* pull requests that don't pass the automated tests should be refined, unless
failures are caused by incompatibilities inherited from the changes (in case of
an API change, ...) (very rare)
* the contributors should explicitly indicate, if a submission is still work in
progress, and post an update, once the status changes, otherwise maintainers
will assume the submission is ready for review, while it actually isn't
* pending pull requests should be kept conflict free and rebased within
reasonable time, unless they are on hold
* if the submission is still "work in progress", but the contributor no longer
wants to continue, then this should be communicated, so that someone else can
pick it up

##### Maintainers

* maintainers should welcome and encourage submissions, especially from new
users
* maintainers should label pull requests with "ready for review", "work in
progress" or "on hold", and update the labels accordingly, once the status
changes
* maintainers may signal a first impression early, especially if it's
foreseeable that a submission won't be accepted
* stalled pull requests, which are likely not going to be improved, should
either be replaced, closed, or put "on hold"

## Reviewing Contributions

##### Contributors

* "work in progress" may be left alone until the status is upgraded, but
comments are welcome nevertheless
* comments on pull requests should primarily focus on the code, and concepts
should be discussed in one or more (newly created) related issues
* nit picking helps to improve the submission, and should not considered as
offense (it's by no means intended as such!)

##### Maintainers

* maintainers should signal the time needed for review, in case it may take
longer than longer than 48 hours
* in more lengthy, or time consuming cases, maintainers, and likewise
contributors, should frequently signal the ongoing progress, including
some rough time estimates, and what may still be needed to finalize the
submission
* maintainers should not ask for other improvements, which are not directly
related to the original submission
* "ready for review" marked pull requests can be merged after peer-review and
ACKs of at least two maintainers (unless it's really trivial)
* NACKs are perfectly fine, but should include a few words describing what lead
to the NACK, so that contributors have a chance to either improve a PR, or gain
an insight for future submissions

## What's next?

The Omni Core team is continuously looking forward to improve the protocol, the
software and the process on the way.

If you are looking for something that isn't mentioned in this document, or
simply want to drop a note or feedback, please feel free to reach out to the
project maintainers.

## Glossary

From time to time contributors or maintainers use abbreviations, and it's likely
that you stumble over one of the following at some point:

* ACK: agreement with an idea, change or submission
* NACK: disagreement or rejection of a proposal
* NIT: comment on an almost trivial issue
* PR: pull request
* WIP: work in progress
* maintainer: project administrator, collaborator
* contributor: someone who submits pull requests, issues, comments, etc.
